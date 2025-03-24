#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include "slab.hh"

#include <rd_utils/concurrency/mutex.hh>

namespace kv_store {
        class HybridKVStore;
}

namespace kv_store::memory {

        struct SlabUsageInfo {
                uint64_t nbHits;
                uint64_t lastTouch;
                bool markedVirtualEvict;
        };

        /**
         * The meta ram collection stores the informations of the keys stored in the RAM
         */
        class MetaRamCollection {

                // The number of slabs that can be managed by the collection
                uint32_t _maxNbSlabs;

                uint32_t _slabTTL;

                uint64_t _currentTime;

                // The RAM slabs storing KVs
                std::map <uint32_t, std::shared_ptr <KVMapRAMSlab> > _loadedSlabs;

                // The list of slabs containing a key
                std::unordered_map <uint64_t, std::unordered_map <uint32_t, uint32_t> > _slabHeads;

                // The usage of slabs
                std::unordered_map <uint32_t, SlabUsageInfo>  _used;


        public:

                /**
                 * The maximum number of slabs that can be stored in the collection
                 */
                MetaRamCollection (uint32_t maxNbSlabs, uint32_t slabTTL = 20);

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ===================================          COLLECTION          ===================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */


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

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ================================          EVICTION/RESIZE          =================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Evict a slab and write it to disk
                 **/
                void evictSlab (HybridKVStore & store);

                /**
                 * Set the maximum number of slabs available in memory
                 * Remove marked slabs in priority (during markOldSlab)
                 */
                void setNbSlabs (uint32_t nbSlabs);

                /**
                 * Mark old slabs for 'virtual' eviction
                 */
                void markOldSlabs (uint64_t currentTime);

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ====================================          GETTERS          =====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * @returns: the memory size allocated to the meta collection
                 */
                rd_utils::utils::MemorySize getMemorySize () const;

                /**
                 * @returns: the memory size reserved by the meta collection
                 */
                rd_utils::utils::MemorySize getMemoryUsage () const;

                /**
                 * @returns: the maximum number of slabs available in RAM
                 */
                uint32_t getNbSlabs () const;

                /**
                 * @returns: the number of slabs allocated in RAM
                 */
                uint32_t getNbLoadedSlabs () const;

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

                /**
                 * @returns: the slab the less used in memory
                 */
                std::shared_ptr <KVMapRAMSlab> getLessUsed ();

                /**
                 * Remove a slab from the list of loaded slabs
                 */
                std::set <uint64_t> removeSlab (uint32_t slabId);

        };

}
