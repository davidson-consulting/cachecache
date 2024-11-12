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
    MemorySize market = this->_size;

    auto allocated = this->sellBaseMemory(market, buyers);
    
    MemorySize allNeeded = MemorySize::MB(0);
    this->buyExtraMemory(allocated, buyers, market, allNeeded);

    for(auto& [id, cache]: this->_entities) {
      cache.last = cache.size;
      cache.size = allocated[id];

      LOG_INFO("Cache ", id, " Req ", cache.req.kilobytes(), "kb - Usage ", cache.usages.current().kilobytes(), "kb - Size ", cache.last.kilobytes(), ">", cache.size.kilobytes(), "kb - Wallet", cache.wallet.kilobytes());
    }
  }

  std::map<uint64_t, MemorySize> Market::sellBaseMemory (
                                                         MemorySize& market
                                                         , std::map<uint64_t, MemorySize>& buyers) {
    
    std::map<uint64_t, MemorySize> allocated;
    MemorySize max = market;

    for (auto & [id, cache]: this->_entities) {
        const MemorySize usage = cache.usages.current();
        const MemorySize requested = cache.req;
        const MemorySize size = cache.size;

        if (usage > size) LOG_DEBUG("OVERUSAGE FOR CACHE ", id, " : ", usage.megabytes() , " > ", size.megabytes()); 

        float percUsage = size.bytes() == 0 ? 0 : (float) usage.bytes() / (float) size.bytes();
        HistoryTrend trend = cache.usages.trend((float)MemorySize::KB(1).bytes() * 0.1); 
        
        // Can't allocate less than a Slab (4MB)
        MemorySize min = MemorySize::MB(4);

        MemorySize allocation = size;
        if (percUsage > this->_triggerIncrement && trend == HistoryTrend::INCREASING) {
            auto previous = usage;
            uint64_t incrementFactor = (1.0 + this->_increasingSpeed) * 100;
            allocation = std::max(min, std::min(max, size * incrementFactor / 100));
        } else if (percUsage < this->_triggerDecrement) {
            uint64_t decrementFactor = (1.0 - this->_decreasingSpeed) * 100;
            allocation = std::max(min, std::min(max, size * decrementFactor / 100));
        }

        if (allocation > requested) {
            allocated[id] = requested;
            buyers[id] = allocation - requested;
        } else {
            allocated[id] = allocation;
            this->increaseWallet(id, requested - allocation);
        }

        market -= allocated[id]; 
    }

    return allocated;
  }

  void Market::buyExtraMemory (std::map<uint64_t, MemorySize> & allocated,
                               std::map<uint64_t, MemorySize> & buyers,
                               MemorySize & market,
                               MemorySize & allNeeded) {

    std::map<uint64_t, MemorySize> failed;

    MemorySize defaultWindowSize = MemorySize::MB(0);
    if (market.bytes() > 0 && buyers.size() > 0) {
      defaultWindowSize = MemorySize::B(market.bytes() / buyers.size());
    }

    while (market.bytes() > 0 && buyers.size() > 0) {

        for (auto cache = buyers.cbegin(); cache != buyers.cend(); ) {
            auto money = this->_entities[cache->first].wallet;
            if (cache->second.bytes() != 0) {
                MemorySize windowSize = std::min(defaultWindowSize, money);

                auto bought = std::min(std::min(windowSize, cache->second), market);
                if (bought.bytes() != 0) {
                    this->_entities[cache->first].wallet = money - bought;
                    buyers[cache->first] -= bought;
                    allocated[cache->first] = allocated[cache->first] + bought;
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

  void Market::increaseWallet(uint64_t cacheId, MemorySize amount) {
    this->_entities[cacheId].wallet += amount;
  }
}
