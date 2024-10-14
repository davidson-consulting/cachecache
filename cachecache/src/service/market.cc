#include <service/market.hh>
#include "cachelib/allocator/memory/Slab.h"


using namespace cachecache;

Market::Market() {}

void Market::configure(const MarketConfig& cfg, Metrics* metrics) {
    this->_memory = cfg.memory;
    this->_triggerIncrement = cfg.triggerIncrement;
    this->_triggerDecrement = cfg.triggerDecrement;
    this->_increasingSpeed = cfg.increasingSpeed;
    this->_decreasingSpeed = cfg.decreasingSpeed;
    this->_windowSize = cfg.windowSize;

    this->_metrics = metrics;
}

void Market::register_cache(const std::string& name, Cachecache* cache) {
    XLOG(INFO, "Register new cache for market ", name);
    this->_caches[name] = cache;
    this->_wallets[name] = 0;
}

void Market::unregister_cache(const std::string& name) {
    this->_caches.erase(name);
    this->_wallets.erase(name);
}

void Market::work() {
    auto & caches = this->_caches;
    for (auto & [name, cache]: caches) {
        XLOG(INFO, "Cache ", name, " using ", cache->currentMemoryUsage(), " - wallet = ", this->_wallets[name]);
        this->_metrics->push("wallet", {{"client", name}}, std::to_string(this->_wallets[name]));
    }

    // sell what's need to be sold
    std::unordered_map<std::string, size_t> buyers;
    size_t market = this->_memory;

    auto allocated = this->sellBaseMemory(market, buyers);

    size_t allNeeded = 0;
    this->buyExtraMemory(allocated, buyers, market, allNeeded);

    /*if (market > 0) {
        size_t notSold = market;
        XLOG(INFO, "Extra = ", notSold, " nb of buyers ", buyers.size());
        long rest = std::min(allNeeded, market);
        for (auto & [name, cache]: this->_caches) {
            float percent = (float) cache->requested() / (float) allNeeded;
            size_t add = (size_t) (percent * rest);
            allocated[name] += add;
            notSold -= add;
        }
    }*/

    for (auto & [name, amount]: allocated) {
        this->_caches[name]->resize(amount);
    }
}

void Market::buyExtraMemory(std::unordered_map<std::string, size_t> & allocated, 
        std::unordered_map<std::string, size_t> & buyers,
        size_t & market,
        size_t & allNeeded) { 
    std::unordered_map<std::string, size_t> failed;
    
    XLOG(INFO, "BUYEXTRAMEMORY market ", market, " buyers ", buyers.size());
    while (market > 0 && buyers.size() > 0) {
        for (auto cache = buyers.cbegin(); cache != buyers.cend(); ) {
            auto money = this->_wallets[cache->first];
            if (cache->second != 0) {
                size_t windowSize = std::min(this->_windowSize, money);

                auto bought = std::min(std::min(windowSize, cache->second), market);
                XLOG(INFO, "CACHE ", cache->first, " BOUGHT ", bought, " ( windowSize ", this->_windowSize, ", money ", money, ", amount ", cache->second, ", market ", market, ")");
                if (bought != 0) {
                    this->_wallets[cache->first] = money - bought;
                    buyers[cache->first] -= bought;
                    allocated[cache->first] = allocated[cache->first] + bought;
                    this->_metrics->push("memory_bought", {{"client", cache->first}}, std::to_string(bought));
                    market -= bought;
                    cache++;
                } else {
                    failed.emplace(cache->first, cache->second);
                    allNeeded += cache->second;
                    buyers.erase(cache++);
                }
            } else {
                buyers.erase(cache++);
            }
        }
    }

    buyers = std::move(failed);
}


std::unordered_map<std::string, size_t> Market::sellBaseMemory(size_t & market, 
        std::unordered_map<std::string, size_t> & buyers) {

    std::unordered_map<std::string, size_t> allocated;
    size_t max = market;

    for (auto & [name, cache]: this->_caches) {
        size_t usage = cache->currentMemoryUsage();
        size_t requested = cache->requested();
        size_t capp = cache->size();

        if (usage > capp) XLOG(ERR, "OVERUSAGE FOR CACHE ", name, " ", usage , " > ", capp);

        float percUsage = (float) usage / (float) capp;

        size_t min = facebook::cachelib::Slab::kSize;

        if (percUsage > this->_triggerIncrement) {
            auto previous = usage;
            usage = std::max(min, std::min(max, (size_t) (usage * (1.0 + this->_increasingSpeed))));
            XLOG(INFO, "TRIGGER INCREMENT FOR ", name, " GOING FROM ", previous, " TO ", usage, " (will go to ", this->_caches[name]->getUpperResizeTarget(usage), ")");
        } else if (percUsage < this->_triggerDecrement) {
            usage = std::max(min, std::min(max, (size_t) (usage * (1.0 - this->_decreasingSpeed))));
        }

        usage = this->_caches[name]->getUpperResizeTarget(usage);
        XLOG(INFO, "CACHE ", name, " usage ", usage, " requested ", requested, " capp ", capp);
        if (usage > requested) {
            allocated[name] = requested;
            buyers[name] = usage - requested;
        } else {
            allocated[name] = usage;
            this->increaseMoney(name, requested - usage);
        }

        market -= allocated[name]; 
    }

    return allocated;
}

void Market::increaseMoney(const std::string& cache, size_t amount) {
    auto fnd = this->_wallets.find(cache);
    if (fnd == this->_wallets.end()) {
        this->_wallets[cache] = amount;
    } else {
        this->_wallets[cache] += amount;
    }
}
