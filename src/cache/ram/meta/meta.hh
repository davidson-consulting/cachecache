#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include "../slab.hh"

#include <rd_utils/concurrency/mutex.hh>

namespace kv_store {
        class HybridKVStore;
}

namespace kv_store::memory { 
        /**
         * The meta ram collection stores the informations of the keys stored in the RAM
         */
        class MetaRamCollection {
        public:
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
                virtual bool insert (const common::Key & k, const common::Value & v) = 0;

                /**
                 * Find the key in the collection
                 */
                virtual std::shared_ptr <common::Value> find (const common::Key & k) = 0;

                /**
                 * Remove the key in the collection
                 * @info: does nothing if the key is not found
                 */
                virtual void remove (const common::Key & k) = 0;

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
                virtual void evictSlab (HybridKVStore & store) = 0;

                /**
                 * Set the maximum number of slabs available in memory
                 * Remove marked slabs in priority (during markOldSlab)
                 */
                virtual void setNbSlabs (uint32_t nbSlabs) = 0;

                /**
                 * Mark old slabs for 'virtual' eviction
                 */
                virtual void markOldSlabs (uint64_t currentTime) = 0;

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
                virtual rd_utils::utils::MemorySize getMemorySize () const = 0;

                /**
                 * @returns: the memory size reserved by the meta collection
                 */
                virtual rd_utils::utils::MemorySize getMemoryUsage () const = 0;

                /**
                 * @returns: the maximum number of slabs available in RAM
                 */
                virtual uint32_t getNbSlabs () const = 0;

                /**
                 * @returns: the number of slabs allocated in RAM
                 */
                virtual uint32_t getNbLoadedSlabs () const = 0;

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ======================================          MISC          ======================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                friend std::ostream & operator<< (std::ostream & s, const MetaRamCollection & mp);
        };

}
