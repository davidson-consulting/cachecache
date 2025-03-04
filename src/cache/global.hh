#pragma once

#include "ram/meta.hh"
#include "disk/meta.hh"
#include <rd_utils/concurrency/mutex.hh>
#include <map>

namespace kv_store {

        class HybridKVStore {
        private:

                struct Promotion {
                        common::Key k;
                        common::Value v;
                };

        private:

                // The collection
                memory::MetaRamCollection _ramColl;

                // The collection on the disk
                disk::MetaDiskCollection _diskColl;

                // The routine time incrementation
                uint64_t _currentTime;

                // Ram collection contains slabs
                bool _hasRam = false;

                // The list of k/v to promote from disk to slab
                std::map <std::string, std::string> _promotions;

                // Mutex to lock promotion set
                rd_utils::concurrency::mutex _promoteMutex;
                rd_utils::concurrency::mutex _ramMutex;
                rd_utils::concurrency::mutex _diskMutex;

        public :

                /**
                 * @params:
                 *    - maxRamSlabs: the number of slabs in the RAM
                 */
                HybridKVStore (uint32_t maxRamSlabs, uint32_t slabTTL);

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * =================================          CONFIGURATION          ==================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Resize the ram collection
                 * @params:
                 *   - maxRamSlabs: the number of slabs in the ram collection
                 */
                void resizeRamColl (uint32_t maxRamSlabs);

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
                void insert (const common::Key & k, const common::Value & v);

                /**
                 * Find a value in the collection
                 */
                std::shared_ptr <common::Value> find (const common::Key & k);

                /**
                 * Remove a key from the collection
                 */
                void remove (const common::Key & k);


                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ====================================          GETTERS          =====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * @returns: the disk collection
                 */
                memory::MetaRamCollection& getRamColl ();

                /**
                 * @returns: the disk collection
                 */
                disk::MetaDiskCollection& getDiskColl ();

                /**
                 * @returns: the disk collection
                 */
                const memory::MetaRamCollection& getRamColl () const;

                /**
                 * @returns: the disk collection
                 */
                const disk::MetaDiskCollection& getDiskColl () const;

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ======================================          MISC          ======================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Update the collection to make the memory usage decrease if possible
                 */
                void loop ();

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ====================================          ROUTINE          =====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                friend std::ostream & operator<< (std::ostream & s, const HybridKVStore & mp);

        };

}
