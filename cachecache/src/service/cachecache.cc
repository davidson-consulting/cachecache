#include "cachecache.hh"
#include "folly/logging/xlog.h"
#include "folly/logging/LogLevel.h"
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>
#include "cachelib/allocator/memory/MemoryPool.h"
#include "cachelib/allocator/LruTailAgeStrategy.h"
#include "cachelib/common/Exceptions.h"
#include "cachelib/allocator/memory/Slab.h"

using namespace rd_utils::concurrency;
using namespace std::chrono;
using namespace cachecache;

using Cache = facebook::cachelib::LruAllocator;
using CacheConfig = typename Cache::Config;
using MMConfig = typename Cache::MMConfig;
using CacheKey = typename Cache::Key;
using CacheHandle = typename Cache::WriteHandle;

Cachecache::Cachecache() {}

Cachecache::~Cachecache() {
    XLOG(INFO, "Nb requests:", this->_reqs_total,  ". Hit ratio: ", static_cast<float>(this->_hits) / static_cast<float>(this->_reqs) * 100);
    this->_gCache.reset();
}

Cachecache::Cachecache(Cachecache && other):
    _name(std::move(other._name))
    , _gCache(std::move(other._gCache))
    , _defaultPool(std::move(other._defaultPool))
    , _clock(std::move(other._clock))
    , _percentiles(std::move(other._percentiles))
    , _hits(other._hits)
    , _reqs(other._reqs)
    , _reqs_total(other._reqs_total)
    , _target(other._target)
    , _metrics(std::move(other._metrics))
{
    other._hits = 0;
    other._reqs = 0;
    other._reqs_total = 0;
    other._target = 0;
}

void Cachecache::operator=(Cachecache && other) {
    this->_name = std::move(other._name);
    this->_gCache = std::move(other._gCache);
    this->_defaultPool = std::move(other._defaultPool);
    this->_clock = std::move(other._clock);
    this->_percentiles = std::move(other._percentiles);
    this->_hits = other._hits;
    other._hits = 0;
    this->_reqs = other._reqs;
    other._reqs = 0;
    this->_reqs_total = other._reqs_total;
    other._reqs_total = 0;
    this->_target = other._target;
    this->_metrics = std::move(other._metrics);
}

void Cachecache::configure(const std::string& name, size_t cachesize, size_t requested, double p0, double p1, double p2, Clock* clock, Metrics* metrics) {
    this->_name = name;
    this->_clock = clock;
    this->_metrics = metrics;
    this->_percentiles[0].setPercentile(p0);
    this->_percentiles[1].setPercentile(p1);
    this->_percentiles[2].setPercentile(p2);

    this->_requested = requested;

    CacheConfig config;
    config
        .setCacheSize(cachesize)
        .setCacheName("Cachecache")
        .setAccessConfig(
            {25 /* bucket power */, 10 /* lock power */}) // assuming caching 20
                                                            // million items
        .validate(); // will throw if bad config

    this->_gCache = std::make_unique<Cache>(config);
    
    facebook::cachelib::LruTailAgeStrategy::Config cfg;
    cfg.slabProjectionLength = 0; // dont project or estimate tail age
    cfg.numSlabsFreeMem = 1;     // ok to have ~40 MB free memory in unused allocations
    this->_gCache->startNewPoolResizer(std::chrono::milliseconds(500), 99999, std::make_shared<facebook::cachelib::LruTailAgeStrategy>(cfg));
    
    const auto allocSize = facebook::cachelib::Slab::kSize;
    const std::set<uint32_t> allocSizes{static_cast<uint32_t>(allocSize)};

    MMConfig mmConfig;
    mmConfig.lruRefreshTime = 0;
    mmConfig.updateOnRead = true;
    mmConfig.updateOnWrite = true;
    this->_defaultPool = this->_gCache->addPool(
                "default", 
                this->_gCache->getCacheMemoryStats().ramCacheSize, {}, 
                mmConfig
            );

    //this->resize(requested);
}

bool Cachecache::resize(size_t newsize) {
    size_t current = this->_gCache->getPool(this->_defaultPool).getPoolSize();
    XLOG(INFO, "Ask to resize from ", current, " to ", newsize, ". Will resize to ", this->getLowerResizeTarget(newsize));
    newsize = this->getLowerResizeTarget(newsize);
    
    if (current == newsize) return true;
    
    if (current > newsize) {
        if (!this->_gCache->shrinkPool(this->_defaultPool, current - newsize)) return false;

        size_t currentMemoryUsage = this->currentMemoryUsage();
        if (currentMemoryUsage > newsize) this->shrink(this->currentMemoryUsage() - newsize);
        return true;
    }
    
    return this->_gCache->growPool(this->_defaultPool, newsize - current);
}

size_t Cachecache::getUpperResizeTarget(size_t target) const {
    //return ceil(target / facebook::cachelib::Slab::kSize) * facebook::cachelib::Slab::kSize;
    return ((target + (facebook::cachelib::Slab::kSize - 1)) / facebook::cachelib::Slab::kSize) * facebook::cachelib::Slab::kSize; 
}

size_t Cachecache::getLowerResizeTarget(size_t target) const {
    return std::max((size_t) 1, target / facebook::cachelib::Slab::kSize) * facebook::cachelib::Slab::kSize;
}

void Cachecache::shrink(size_t amount) {
    XLOG(INFO, "############ SHRINK at time ", this->_clock->time()); 
    
    int target_in_slabs = (amount / facebook::cachelib::Slab::kSize) + 1;
    XLOG(INFO, "Target ", amount, " Target in slabs ", target_in_slabs);
    this->_gCache->updateNumSlabsToAdvise(target_in_slabs);
    auto results = this->_gCache->calcNumSlabsToAdviseReclaim();
    facebook::cachelib::LruTailAgeStrategy strategy;

    if (results.advise) {
        XLOG(INFO, "REMOVING");
        for (auto& result : results.poolAdviseReclaimMap) {
          uint64_t slabsAdvised = 0;
          facebook::cachelib::PoolId poolId = result.first;
          XLOG(INFO, "POOL ID ", poolId);
          uint64_t slabsToAdvise = result.second;
          while (slabsAdvised < slabsToAdvise) {
            const auto classId = strategy.pickVictimForResizing(*this->_gCache, poolId);
            if (classId == facebook::cachelib::Slab::kInvalidClassId) {
              XLOG(INFO, "Invalid class id");
              break;
            }
            try {
                this->_gCache->releaseSlab(poolId, classId, facebook::cachelib::SlabReleaseMode::kAdvise);
                ++slabsAdvised;
            } catch (const facebook::cachelib::exception::SlabReleaseAborted& e) {
              XLOG(INFO,
                    "Aborted trying to advise away a slab from pool {} for"
                    " allocation class {}. Error: {}",
                    static_cast<int>(poolId), static_cast<int>(classId), e.what());
              return;
            } catch (const std::exception& e) {
              XLOG(
                  INFO,
                  "Error trying to advise away a slab from pool {} for allocation "
                  "class {}. Error: {}",
                  static_cast<int>(poolId), static_cast<int>(classId), e.what());
            }
          }
          XLOGF(INFO, "Advised away {} slabs from Pool ID: {}, to free {} bytes",
                slabsAdvised, static_cast<int>(poolId), slabsAdvised * facebook::cachelib::Slab::kSize);
        }
    }
}

size_t Cachecache::size() const {
    return this->_gCache->getPool(this->_defaultPool).getPoolSize();
}

size_t Cachecache::requested() const {
    return this->_requested;
}

bool Cachecache::get(CacheKey key) { 
    CacheHandle item;

    try {
        item = this->_gCache->findToWrite(key);
    } catch (std::exception& e) {
        XLOG(ERR, "Could not find key ", key, " - ", e.what());
    }

    this->_reqs++;
    this->_reqs_total++;
    if(item == nullptr) {
        // if not in key buffer, add it
        if (std::find(std::begin(this->_key_buffer), std::end(this->_key_buffer), key) == std::end(this->_key_buffer)) {
            this->_key_buffer[this->_key_buffer_index] = key;
            this->_key_buffer_index = (this->_key_buffer_index + 1) % 10000;
        }
        return false;
    }
    
    this->_hits++;

    folly::StringPiece sp{reinterpret_cast<const char*>(item->getMemory()),
                          item->getSize()};

    ITEM i = from_string(sp.str()); 
    std::string value = i.value;
    unsigned int last = i.last_request;
    this->_metrics->push("delta", {{"client", this->_name}}, std::to_string(this->_clock->delta(last)));
    
    if (this->_calibrating) {
        for(auto& percentile: this->_percentiles) {    
            percentile.addValue(this->_clock->delta(last));
        }
    } 

    i.last_request = this->_clock->time();
    //XLOG(INFO, "Last request (GET) ", last, " -> ", i.last_request);

    std::string stritem = to_string(i);

    if (item->getSize() >= stritem.size()) {
        std::memcpy(item->getMemory(), stritem.c_str(), stritem.size());
        return true;
    }

    try {
        auto handle = this->_gCache->allocate(this->_defaultPool, key, stritem.size());
        std::memcpy(handle->getMemory(), stritem.c_str(), stritem.size());
        this->_gCache->insertOrReplace(handle);
    } catch (const std::exception& e) {
        XLOG(ERR, "Could not update item : ", e.what());
    }
    return true;
}

bool Cachecache::put(CacheKey key, const std::string& value) {
    ITEM item;
    item.value = value;
    item.last_request = this->_clock->time();
    //XLOG(INFO, "Last request (PUT)", item.last_request);
    std::string stritem = to_string(item); 

    try {
        // if not present in cache nor in key buffer, put it in key buffer
        /*if (this->_gCache->findFast(key) == nullptr && std::find(std::begin(this->_key_buffer), std::end(this->_key_buffer), key) == std::end(this->_key_buffer)) {
            this->_key_buffer[this->_key_buffer_index] = key;
            this->_key_buffer_index = (this->_key_buffer_index + 1) % 10000;
            return true;
        }*/

        auto handle = this->_gCache->allocate(this->_defaultPool, key, stritem.size());
        
        if (!handle) {
            XLOG(ERR, "Could not allocate.");
            return false; // cache may fail to evict due to too many pending writes
        }

        std::memcpy(handle->getMemory(), stritem.c_str(), stritem.size());
        this->_gCache->insertOrReplace(handle);
    } catch (const std::exception& e) {
        XLOG(ERR, "Key ", key);
        XLOG(ERR, "Could not allocate : ", e.what());
        return false;
    }
    return true;
}

int Cachecache::clean(Thread t) {
    return this->clean();
}

int Cachecache::clean() {
    XLOG(INFO, "Clean at time ", this->_clock->time());
    /*for(const auto& percentile: this->_percentiles) {
        XLOG(INFO,"Percentile: ", percentile.getIndex(), " = ", percentile.getEstimation(), " (count = ", percentile.getCount(), ")");
    }*/
    for(int i = 0; i < 3; i++) {
        const auto& percentile = this->_percentiles[i];
        XLOG(INFO,"Percentile: ", percentile.getIndex(), " = ", percentile.getEstimation(), " (count = ", percentile.getCount(), ")");
    }

    double perc_mem_usage = (double) this->currentMemoryUsage() / (double) this->requested();
    XLOG(INFO, "Percentage memory usage ", perc_mem_usage * 100); 

    if (perc_mem_usage <= 0.5) {
        this->_calibrating = true;
        return 0;
    }

    this->_calibrating = false;

    if (perc_mem_usage <= 0.8) this->_targetedPercentile = 2;
    if (perc_mem_usage <= 0.9) { 
        this->_targetedPercentile = 1;
    } else {
        this->_targetedPercentile = 0;
    }

    XLOG(INFO, "Targeted percentile ", this->_targetedPercentile); 
    
    size_t before = 0;

    try {
        before = this->_gCache->getPool(this->_defaultPool).getCurrentAllocSize();
    } catch (const std::exception& e) {
        XLOG(ERR, "Could not get current alloc size for cache : ", e.what());
        return 0;
    }
    int nb_keys_removed = 0;
    int nb_keys_used_removed = 0;

    this->_target = this->_percentiles[this->_targetedPercentile].getEstimation() * 1.1;
    
    XLOG(INFO, "Target ", this->_target);
    this->_metrics->push("eviction_target", {{"client", this->_name}}, std::to_string(this->_target));

    auto start = high_resolution_clock::now();
    
    try {
        for(const auto& id: this->_gCache->getPool(this->_defaultPool).getStats().classIds) {
            auto& container = this->_gCache->getMMContainer(this->_defaultPool, id);
            
            std::vector<facebook::cachelib::KAllocation::Key> to_remove;
            for(auto itr = container.getEvictionIterator(); itr; ++itr) {
                folly::StringPiece sp{reinterpret_cast<const char*>(itr->getMemory()),
                              itr->getSize()};

                ITEM item = from_string(sp.str());
                if(this->_clock->delta(item.last_request) <= this->_target) break;
                if(item.last_request != 0) {
                    nb_keys_used_removed++;
                }

                to_remove.push_back(itr->getKey());
            }
            for(auto key: to_remove) {
                this->_gCache->remove(key);
                nb_keys_removed++;
            }
        }
    } catch (const std::exception& e) {
        XLOG(ERR, "Could not clean cache : ", e.what());
    }
    
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start).count();
  
    this->_metrics->push("nb_evictions", {{"client", this->_name}}, std::to_string(nb_keys_removed));
    this->_metrics->push("time_eviction", {{"client", this->_name}}, std::to_string(duration));

    size_t cachesize = this->_gCache->getPool(this->_defaultPool).getCurrentAllocSize();
    this->_metrics->push("size_eviction", {{"client", this->_name}}, std::to_string(before - cachesize));
    this->_metrics->push("percentage_evictions", {{"client", this->_name}}, std::to_string((before - cachesize) * 100 / before));
    try {
        XLOG(INFO, "Removed ", nb_keys_removed, " keys in ", duration, ". Went from ", before, " to ", cachesize);
    } catch (const std::exception& e) {
        XLOG(ERR, "Could not get cache size : ", e.what());
    } 

    return nb_keys_removed;
}

void Cachecache::push_metrics() {
    this->_metrics->push("hits", {{"client", this->_name}}, std::to_string(this->_hits));
    this->_metrics->push("nb_reqs", {{"client", this->_name}}, std::to_string(this->_reqs));

    this->_hits = 0;
    this->_reqs = 0;

    for (int i = 0; i < 3; i++) {
        this->_metrics->push(
                "percentile", 
                {{"client", this->_name}, {"percentage", std::to_string(this->_percentiles[i].getPercentile())}},
                std::to_string(this->_percentiles[i].getEstimation())
        );
    }
  
    this->_metrics->push("cache_size", {{"client", this->_name}}, std::to_string(this->size()));
    this->_metrics->push("memory_usage", {{"client", this->_name}}, std::to_string(this->currentMemoryUsage()));
}

size_t Cachecache::currentMemoryUsage() const {
    //return this->_gCache->getPool(this->_defaultPool).getCurrentUsedSize();
    return this->_gCache->getPool(this->_defaultPool).getCurrentAllocSize();
}

void Cachecache::setTargetedPercentile(unsigned int i) {
    this->_targetedPercentile = std::min(i, (unsigned int)this->_percentiles.size());
}

ITEM Cachecache::from_string(std::string s) const {
    auto delimiter = s.find(":");
    
    ITEM i;
    i.last_request = this->_clock->from_string(s.substr(0, delimiter));
    i.value = s.substr(delimiter + 1);
    return i;
}

std::string Cachecache::to_string(ITEM i) const {
    return this->_clock->to_string(i.last_request) + ":" + i.value;
}
