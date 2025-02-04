#pragma once

#include "ram/meta.hh"
#include "disk/meta.hh"

namespace kv_store {

        class HybridKVStore {

                // The collection
                memory::MetaRamCollection _ramColl;

                // The collection on the disk
                disk::MetaDiskCollection _diskColl;

        public :

                /**
                 * @params:
                 *    - maxRamSlabs: the number of slabs in the RAM
                 */
                HybridKVStore (uint32_t maxRamSlabs);


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

                /**
                 * Demote a key from RAM to disk
                 */
                void demote (const common::Key & k, const common::Value & v);

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

                friend std::ostream & operator<< (std::ostream & s, const HybridKVStore & mp);

        private:

                /**
                 * Promote a kv from disk to RAM
                 */
                void promote (const common::Key & k, const common::Value & v);


        };

}
