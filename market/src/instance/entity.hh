#pragma once

#include <string>
#include <cstdint>
#include <rd_utils/_.hh>

#include "../cache/global.hh"

namespace kv_store::instance {

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

                // The ttl to set when inserting a key
                uint32_t _ttl;

                // The cache managed by the entity
                std::unique_ptr <kv_store::HybridKVStore> _entity;

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
                 *    - diskSize: the size available on disk for a cache instance
                 */
                void configure (const std::string & name, rd_utils::utils::MemorySize maxSize, rd_utils::utils::MemorySize diskSize, uint32_t slabTTL);

                /**
                 * Resize the cache entity to take a new memory space
                 * @params:
                 *    - newSize: the new size taken by the cache in bytes
                 * @assume: 'newSize' < this-> getMaxSize ()
                 */
                bool resize (rd_utils::utils::MemorySize newSize);

                /**
                 * Update the memory usage and clear what can be cleared
                 */
                void loop () ;

                /**
                 * @returns: the maximum size that can be taken by the cache
                 */
                rd_utils::utils::MemorySize getMaxSize () const;

                /**
                 * @returns: the current size of the main cache pool
                 *      (can be lower than maximum size as cache get
                 *      reconfigured by the supervisor)
                 */
                rd_utils::utils::MemorySize getSize () const;

                /**
                 * @returns: the memory actually used by the cache
                 */
                rd_utils::utils::MemorySize getCurrentMemoryUsage () const;

                /**
                 * @returns: the memory actually used by the cache on disk
                 */
                rd_utils::utils::MemorySize getCurrentDiskUsage () const;

                /**
                 * Insert a new key value
                 * @params:
                 *    - key: the key to insert
                 *    - session: the session that will send the value
                 */
                bool insert (const std::string & key, rd_utils::net::TcpStream& session);

                /**
                 * Retreive a value
                 * @params:
                 *    - key: the key to find
                 *    - session: the session to which the value will be sent
                 * @returns:
                 *    - true iif found
                 */
                bool find (const std::string & key, rd_utils::net::TcpStream& session, bool & onDisk);

                /**
                 * Dispose all content of the cache, and free all handles
                 */
                void dispose ();

                /**
                 * @cf: this-> dispose ();
                 */
                ~CacheEntity ();

        };


}
