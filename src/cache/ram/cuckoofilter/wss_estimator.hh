#pragma once

#include <array>
#include "cuckoofilter.hh"
#include "table.hh"
#include "../../common/key.hh"

namespace kv_store::memory::wss {
    // This class estimate the WSS of a given segment
    class WSSEstimator {
        class Distribution {
            public:
                Distribution() = default;

                /**
                 * @returns the number of buckets
                 */ 
                constexpr int nb_buckets() const;

                /**
                 *  @params
                 *  - i: the index of the bucket 
                 *
                 *  @returns the number of elements in the 
                 *  bucket i
                 */
                int nb_in(int i) const;

                /**
                 *  @params
                 *  - i: the index of the bucket
                 *
                 *  @returns the size of the bucket
                 */
                int size_of(int i) const;

                /**
                 * @params
                 * - i: the index of the bucket
                 *
                 *  @returns the threshold of the given bucket
                 */
                int threshold_of(int i) const;

                void insert(const kv_store::memory::cuckoofilter::Item& i);
                void update(const kv_store::memory::cuckoofilter::Item& i);
                void remove(const kv_store::memory::cuckoofilter::Item& i);

            private:
                // Bin threshold
                std::array<int, 3> _get_stages = {0, 1, 10};
                // Number of keys for each bin
                std::array<int, 3> _distribution = {0, 0, 0};
                // Sum of key sizes for each bin
                std::array<uint32_t, 3> _sizes = {0, 0, 0};

                // Move a key from a bucket to another
                void move(int from, int to, const kv_store::memory::cuckoofilter::Item& item);

                // Add the item to the i'nth bucket
                void add_to_bucket(int i, const kv_store::memory::cuckoofilter::Item& item);
                // Remove the item from the i'nth bucket
                void remove_from_bucket(int i, const kv_store::memory::cuckoofilter::Item& item);
        };
        public:
        WSSEstimator(size_t max_nb_keys);

        /**
         *  Insert a key and update the distribution
         */
        void insert(const common::Key &key, uint32_t value_size);

        /**
         *  Read a key (and potentially update the distribution,
         *  depending on the current corresponding number of access
         *  and the configured threshold)
         */
        void find(const common::Key &key);

        /**
         *  Remove a key and update the distribution
         */ 
        void del(const common::Key &key);

        /**
         * Return the current WSS size
         * (the WSS excluse the never accessed items)
         */ 
        uint32_t wss() const;

        /**
         * Return the current size
         */ 
        uint32_t size() const;

        private:
        kv_store::memory::cuckoofilter::CuckooFilter<16> _filter;
        Distribution _distribution;
    }; 
}
