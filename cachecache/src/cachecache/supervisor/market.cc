#define LOG_LEVEL 10

#include <rd_utils/utils/log.hh>
#include "cachelib/allocator/memory/Slab.h"
#include "market.hh"

using namespace rd_utils::utils;

namespace cachecache::supervisor {

  Market::Market (MemorySize size) :
    _size (size)
    , _triggerIncrement (0.8)
    , _triggerDecrement (0.5)
    , _increasingSpeed (0.1)
    , _decreasingSpeed (0.1)
  {}

  Market::Market (const MarketConfig& cfg) :
    _size (cfg.size)
    , _triggerIncrement (cfg.triggerIncrement)
    , _triggerDecrement (cfg.triggerDecrement)
    , _increasingSpeed (cfg.increasingSpeed)
    , _decreasingSpeed (cfg.decreasingSpeed)
  {}

  Market::~Market () {}

  void Market::registerCache (uint64_t uid, MemorySize req, MemorySize usage) {
    if (this-> _entities.find (uid) != this-> _entities.end ()) {
      this-> _entities.erase (uid);
    }

    this-> _entities.emplace (uid, CacheStatus {
        .uid = uid,
        .req = req,
        .size = usage,
        .last = usage,
        .wallet = MemorySize::MB(0)
      });

    this->_entities.at(uid).usages.add(usage);
  }

  void Market::removeCache (uint64_t uid) {
    this-> _entities.erase (uid);
  }

  void Market::updateUsage (uint64_t uid, MemorySize usage) {
    LOG_INFO("UPDATE USAGE - CACHE ", uid, " IS USING ", usage.bytes(), " - ", usage.megabytes());
    auto it = this-> _entities.find (uid);
    if (it != this-> _entities.end ()) {
      it-> second.usages.add(usage);
    }
  }

  void Market::updateSize (uint64_t uid, MemorySize size) {
    auto it = this-> _entities.find (uid);
    if (it != this-> _entities.end ()) {
      it-> second.size = size;
    }
  }

  MemorySize Market::getCacheSize (uint64_t uid) const {
    auto it = this-> _entities.find (uid);
    if (it != this-> _entities.end ()) {
      return it-> second.size;
    }

    return MemorySize::B (0);
  }

  bool Market::hasChanged (uint64_t uid) const {
    auto it = this-> _entities.find (uid);
    if (it != this-> _entities.end ()) {
      return (it-> second.size.bytes () != it-> second.last.bytes ());
    }

    return false;
  }


  void Market::run () {
    std::map<uint64_t, MemorySize> buyers;
    MemorySize market = this-> _size;
    MemorySize zero = MemorySize::B (0);

    /*
     * Market works in four phases,
     *   - 1) compute the needs of the cache instances, and provide them their guaranteed memory size
     */
    auto allocated = this-> sellBaseMemory (market, buyers);

    /*
     *   - 2) run an auction to buy the memory that was sell to no one
     *        and store the buyers (needing more memory than guaranteed) that failed to buy
     */
    MemorySize allNeeded = zero;
    this-> buyExtraMemory (allocated, buyers, market, allNeeded);

    /*
     *   - 3) Distribute the remaining memory to the failing buyers for free
     */
    if (market > zero && allNeeded > zero) {
      auto rest = MemorySize::min (allNeeded, market);
      for (auto & b : buyers) {
        // give them the remaining according to the respective asking size
        auto percent = (float) b.second.bytes () / (float) allNeeded.bytes ();
        auto add = rest * percent;
        allocated [b.first] += add;
        market -= add;
      }
    }

    /*
     *    - 4) Distribute the remaining memory to the cache entities equally
     */
    MemorySize adding = zero;
    if (market > zero && allocated.size () != 0) {
      adding = market / allocated.size ();
    }

    for(auto& [id, cache]: this-> _entities) {
      cache.last = cache.size; // Store the last cache size, to see check for size change
      cache.size = allocated [id] + adding; // set the cache size to what was given to the cache in base, auction, and final distribution

      LOG_INFO ("Cache ", id, " Req ", cache.req.kilobytes(), "kb - Usage ", cache.usages.current().kilobytes(), "kb - Size ", cache.last.kilobytes(), ">", cache.size.kilobytes(), "kb - Wallet", cache.wallet.kilobytes());
    }
  }

  std::map<uint64_t, MemorySize> Market::sellBaseMemory (MemorySize& market
                                                         , std::map<uint64_t, MemorySize>& buyers) {
    std::map<uint64_t, MemorySize> allocated;

    // Can't allocate less than four Slabs (4MB)-> with only one slab the cache tends to fail when allocating
    // And cannot allocate more than the size of the all cache
    MemorySize min = MemorySize::MB (32);
    MemorySize max = market;

    for (auto & [id, cache]: this->_entities) {
      const MemorySize usage = cache.usages.current (); // cache current usage (due to stored Key/Values)
      const MemorySize requested = cache.req; // cache requested size (guarantee)
      const MemorySize size = cache.size; // cache previous size (set by the previous market iteration)

      if (usage > size) LOG_DEBUG ("OVERUSAGE FOR CACHE ", id, " : ", usage.megabytes() , " > ", size.megabytes());

      float percUsage = size.bytes() == 0 ? 0 : (float) usage.bytes() / (float) size.bytes(); // usage over size
      HistoryTrend trend = cache.usages.trend ((float) MemorySize::KB (1).bytes () * 0.1); // compute the trend of the cache /> \> -->
      bool overTrig  = (percUsage > this-> _triggerIncrement) && (trend != HistoryTrend::STEADY);
      bool underTrig = (percUsage < this-> _triggerDecrement) && (trend != HistoryTrend::STEADY);

      /*
       * Base memory selling is computing the needs of the cache entities, and selling the size that goes under their request
       * Each cache needing more than base request is put in the buyers queue
       *
       * There are three cases :
       *    - 1) The cache is increasing its memory usage
       */
      if (overTrig) {
        MemorySize increase = size * (1.0 + this->_increasingSpeed); // increase by 1.x
        MemorySize current = MemorySize::max (min, MemorySize::min (requested, increase)); // memory to set in base selling that does not go over requested

        market -= current;
        allocated [id] = current;
        if (increase > requested) {
          buyers [id] = MemorySize::min (max - requested, increase - requested); // add to the buying queue
        } else {
          // using less the requested, means more money
          this-> increaseWallet (id, requested - increase);
        }
      }

      /*
       *    - 2) The cache is decreasing its memory usage
       */
      else if (underTrig) {
        MemorySize increase = size * (1.0 - this->_decreasingSpeed);
        MemorySize current = MemorySize::max (min, MemorySize::min (requested, increase));

        market -= current;
        allocated [id] = current;
        if (increase > requested) {
          buyers [id] = MemorySize::min (max - requested, increase - requested); // add to the buying queue
        } else {
          // using less the requested, means more money
          this-> increaseWallet (id, requested - increase);
        }
      }

      /*
       *    - 3) The cache use is steady (no big increase/decrease in the pase N seconds), or it just didn't trigger any change
       */
      else {
        auto increase = MemorySize::min (max, usage + (max * 0.01));
        auto current = MemorySize::max (min, MemorySize::min (requested, increase));
        LOG_INFO ("Cache : ", id, " ", increase.kilobytes (), " ", current.kilobytes ());

        market -= current;
        allocated [id] = current; // allocating current usage, or requested
        if (increase > requested) { // Steady but using more memory than requested
          buyers [id] = MemorySize::min (max - requested, increase - requested); // add the rest to the buying queue
        } else { // steady and using less than requested
          this-> increaseWallet (id, requested - increase);
        }
      }
    }

    return allocated;
  }

  void Market::buyExtraMemory (std::map<uint64_t, MemorySize> & allocated,
                               std::map<uint64_t, MemorySize> & buyers,
                               MemorySize & market,
                               MemorySize & allNeeded) {

    if (market.bytes () == 0 || buyers.size () == 0) { // No need for auction there is nothing to sell
      return;
    }

    std::map<uint64_t, MemorySize> failed;
    MemorySize defaultWindowSize = MemorySize::B (market.bytes () / buyers.size ());

    while (market.bytes () > 0 && buyers.size () > 0) {
      for (auto cache = buyers.cbegin (); cache != buyers.cend(); ) { // we cannot use : for (auto & v : buyers), because we need to erase elements in the map
        auto money = this-> _entities [cache-> first].wallet;

        if (cache-> second.bytes() != 0) { // Cache wants to buy some memory
          MemorySize windowSize = MemorySize::min (defaultWindowSize, money);
          auto bought = MemorySize::min (MemorySize::min (windowSize, cache-> second), market);

          if (bought.bytes () != 0) {
            this->_entities [cache->first].wallet = money - bought; // remove the bought memory from the wallet
            buyers [cache->first] -= bought; // remove it from the buying queue
            allocated [cache->first] += bought; // Adding it to the allocation
            market -= bought; // remove it from the market
            cache ++; // iterate to the next cache instance
          } else {
            failed.emplace (cache->first, cache->second); // cache cannot buy, maybe it will have a better change in final stage
            allNeeded += cache->second; // sum of all failed
            buyers.erase (cache++); // remove the cache from the buying queue
          }
        } else {
          buyers.erase (cache++); // the cache bought everything it needed, remove it from the buying queue
        }
      }
    }

    // The buyers that failed to buy their memory needs, for the final phase
    buyers = std::move(failed);
  }

  void Market::increaseWallet(uint64_t cacheId, MemorySize amount) {
    this->_entities[cacheId].wallet += amount;
  }
}
