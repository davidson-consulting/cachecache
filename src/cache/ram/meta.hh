#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include "slab.hh"

namespace kv_store {
        class HybridKVStore;
}

namespace kv_store::memory {

        /**
         * The meta ram collection stores the informations of the keys stored in the RAM
         */
        class MetaRamCollection {

                // The number of slabs that can be managed by the collection
                uint32_t _maxNbSlabs;

                // The RAM slabs storing KVs
                std::map <uint32_t, std::shared_ptr <KVMapRAMSlab> > _loadedSlabs;

                // The list of slabs containing a key
                std::map <uint64_t, std::map <uint32_t, uint32_t> > _slabHeads;

                // The lru of the ram collection
                SlabLRU _lru;

        public:

                /**
                 * The maximum number of slabs that can be stored in the collection
                 */
                MetaRamCollection (uint32_t maxNbSlabs);

                /**
                 * Insert a key value in the collection
                 */
                bool insert (const common::Key & k, const common::Value & v);

                /**
                 * Find the key in the collection
                 */
                std::shared_ptr <common::Value> find (const common::Key & k);

                /**
                 * Remove the key in the collection
                 * @info: does nothing if the key is not found
                 */
                void remove (const common::Key & k);

                /**
                 * Evict kvs in the collection (according to LRU order) until the memory fits
                 * @info: insert the KV values in the memory
                 * @returns:
                 *   - maxAllocable: the maximum allocable size in the slab that freed the memory space
                 *   - k: the key
                 *   - v: the value
                 */
                 bool evictUntilFit (const common::Key & k, const common::Value & v, HybridKVStore & store);

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ======================================          MISC          ======================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                friend std::ostream & operator<< (std::ostream & s, const MetaRamCollection & mp);

        private:

                /**
                 * Insert the meta data information of the key to retreive the key faster
                 */
                void insertMetaData (uint64_t hash, uint32_t slabId);

                /**
                 * Remove a key in the meta data information
                 */
                void removeMetaData (uint64_t hash, uint32_t slabId);

        };

}
