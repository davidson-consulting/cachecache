#pragma once

#include <string>
#include <unordered_map>
#include <cachelib/common/PeriodicWorker.h>
#include <service/cachecache.hh>
#include <service/metrics/metrics.hh>

namespace cachecache {
    class Supervisor;

    struct MarketConfig {
        /// Global cache size
        size_t memory; 

        /// The percentage of usage of memory in VM before an increment of memory allocation
        float triggerIncrement;

        /// The speed of the increment of memory
        float increasingSpeed;

        /// The percentage of usage of memory in VM before an decrement of memory allocation
        float triggerDecrement;

        /// The speed of the decrement of memory
        float decreasingSpeed;

        size_t windowSize;
    };

    class Market : public facebook::cachelib::PeriodicWorker {
        public:
            Market();
            Market(Market &) = delete;
            void operator=(Market &) = delete;
            void configure(const MarketConfig& cfg, Metrics* metrics);
            void register_cache(const std::string& name, Cachecache* cache);
            void unregister_cache(const std::string& name);
            void work(); 

        private:
            // CONFIG
            size_t _memory;
            float _triggerIncrement;
            float _triggerDecrement;
            float _increasingSpeed;
            float _decreasingSpeed;
            size_t _windowSize;

            Metrics* _metrics;

            std::unordered_map<std::string, Cachecache*> _caches;
            // mapping between a cache name and its wallet
            std::unordered_map<std::string, size_t> _wallets;

            std::unordered_map<std::string, size_t> sellBaseMemory(size_t & market, 
                    std::unordered_map<std::string, size_t> & buyers);

            void buyExtraMemory(std::unordered_map<std::string, size_t> & allocated, 
                    std::unordered_map<std::string, size_t> & buyers,
                    size_t & market,
                    size_t & allNeeded);
            void increaseMoney(const std::string& cache, size_t amount);
    };
}
