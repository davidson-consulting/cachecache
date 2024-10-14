#pragma once 

#include <array>
#include <chrono>
#include <string>
#include <unordered_map>
#include <memory>

#include "cachelib/allocator/CacheAllocator.h"
#include <rd_utils/concurrency/thread.hh>
#include <rd_utils/concurrency/mutex.hh>

#include <service/metrics/metrics.hh>
#include <service/clock/clock.hh>
#include <service/percentile.hh>

namespace cachecache {
    struct ITEM {
        std::string value;
        unsigned int last_request;
    };

    class Cachecache {
        public:
            Cachecache();
            ~Cachecache();

            Cachecache(Cachecache &) = delete;
            void operator=(Cachecache &) = delete;

            Cachecache(Cachecache &&);
            void operator=(Cachecache &&);

            void configure(const std::string& name, size_t cachesize, size_t requested, double p0, double p1, double p2, Clock* clock, Metrics* metrics);
            bool resize(size_t newsize);

            size_t getUpperResizeTarget(size_t target) const;
            size_t getLowerResizeTarget(size_t target) const;

            
            bool get(facebook::cachelib::LruAllocator::Key key);
            bool put(facebook::cachelib::LruAllocator::Key key, const std::string& value);

            int clean(rd_utils::concurrency::Thread);
            int clean();

            void push_metrics();

            size_t currentMemoryUsage() const;
            size_t requested() const;
            size_t size() const;

            void setTargetedPercentile(unsigned int i);

            void test();

        private:
            std::string _name;
            size_t _requested;

            std::unique_ptr<facebook::cachelib::LruAllocator> _gCache;
            facebook::cachelib::PoolId _defaultPool;

            Clock* _clock;

            std::array<Percentile, 3> _percentiles;
            std::array<facebook::cachelib::LruAllocator::Key, 10000> _key_buffer;
            int _key_buffer_index = 0;

            unsigned int _targetedPercentile = 2;

            bool _calibrating = true;

            // METRICS
            unsigned long _hits = 0;
            unsigned long _reqs = 0;
            unsigned long _reqs_total = 0;
            double _target = 0;
            
            Metrics* _metrics;

            void shrink(size_t amount);

            std::string to_string(ITEM) const;
            ITEM from_string(std::string) const;
    };
}
