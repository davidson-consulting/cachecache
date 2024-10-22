#pragma once

#include <string>
#include <cstdint>
#include "cachelib/allocator/CacheAllocator.h"
#include <rd_utils/_.hh>

namespace cachecache::instance {

        enum Limits : int64_t {
                MAX_KEY = 2048,
                BASE_TTL = 60
        };

        /**
         * A cache entity is a cachelib instance used as a key/value store
         */
        class CacheEntity {
        private:

                // The uniq name of the entity
                std::string _name;

                // The maximum size that can be taken by the cache
                rd_utils::utils::MemorySize _maxSize;

                // The maximum size a value can be
                rd_utils::utils::MemorySize _maxValueSize;

                // The requested size for the cache instance
                rd_utils::utils::MemorySize _requested;;

                // The ttl to set when inserting a key
                uint32_t _ttl;

                // The cache managed by the entity
                std::unique_ptr <facebook::cachelib::LruAllocator> _entity;

                // Pool to manage the actual size of the cache
                facebook::cachelib::PoolId _pool;

                // Clock* _clock;

                // std::array<Percentile, 3> _percentiles;
                // std::array<facebook::cachelib::LruAllocator::Key, 10000> _key_buffer;

        public:

                /**
                 * ========================================================
                 * ========================================================
                 * ====================   DELETED  ========================
                 * ========================================================
                 * ========================================================
                 */

                CacheEntity (const CacheEntity &) = delete;
                void operator=(const CacheEntity&) = delete;
                CacheEntity (CacheEntity &&) = delete;
                void operator=(CacheEntity &&) = delete;

        public:

                CacheEntity ();

                /**
                 * Configure the cache
                 * @params:
                 *    - name: the uniq name of the cache
                 *    - maxSize: the maximum size the cache can take
                 */
                void configure (const std::string & name, rd_utils::utils::MemorySize maxSize, uint32_t ttl);

                /**
                 * Resize the cache entity to take a new memory space
                 * @params:
                 *    - newSize: the new size taken by the cache in bytes
                 * @assume: 'newSize' < this-> getMaxSize ()
                 */
                bool resize (rd_utils::utils::MemorySize newSize);

                /**
                 * @returns: the maximum size that can be taken by the cache
                 */
                rd_utils::utils::MemorySize getMaxSize () const;

                /**
                 * @returns: the memory actually used by the cache
                 */
                rd_utils::utils::MemorySize getCurrentMemoryUsage () const;

                /**
                 * Insert a new key value
                 * @params:
                 *    - key: the key to insert
                 *    - session: the session that will send the value
                 */
                void insert (const std::string & key, rd_utils::net::TcpSession & session);

                /**
                 * Retreive a value
                 * @params:
                 *    - key: the key to find
                 *    - session: the session to which the value will be sent
                 */
                void find (const std::string & key, rd_utils::net::TcpSession & session);

                /**
                 * Dispose all content of the cache, and free all handles
                 */
                void dispose ();

                /**
                 * @cf: this-> dispose ();
                 */
                ~CacheEntity ();

        private:

                /**
                 * Remove some memory slabs
                 * @params:
                 *    - memSize: the memory size to reclaim
                 */
                void reclaimSlabs (rd_utils::utils::MemorySize memSize);

                /**
                 * Remove an amount of slabs in a given pool
                 * @params:
                 *    - poolId: the pool in which the reclaim has to be made
                 *    - nbSlabs: the number of slabs to reclaim
                 * @returns: the slabs that were successfuly reclaimed
                 */
                uint64_t reclaimSlabsInPool (facebook::cachelib::PoolId poolId, uint64_t nbSlabs);

                /**
                 * Allocate the cachelib cache instance
                 * @params:
                 *    - name: the name of the cache
                 *    - size: the maximum memory size the cache can use (in bytes)
                 *    - ttl: the default ttl set on keys
                 */
                void configureEntity (const std::string & name, rd_utils::utils::MemorySize maxSize, uint32_t ttl);

                /**
                 * Configure the LRU behavior of the cache entity
                 */
                void configureLru ();

                /**
                 * Configure the memory pool of the cache
                 * The memory pool is used to resize the cache
                 */
                void configurePool (const std::string & name);

                /**
                 * @returns: the size of a slab (minimum size of the cache)
                 */
                static rd_utils::utils::MemorySize getSlabSize ();

        };


}
