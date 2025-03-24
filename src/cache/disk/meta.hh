#pragma once

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <list>
#include "slab.hh"
#include "map.hh"

#include "../ram/slab.hh"


namespace kv_store::disk {

        /**
         * The meta ram collection stores the informations of the keys stored in the Disk
         */
        class MetaDiskCollection {

                // The free list used to manage the meta structure
                DiskMap _metaColl;

                std::set <uint64_t> _notFullSlabs;

                // The maximum number of slabs that can be written on disk
                uint64_t _maxNbSlabs;

                // The current number of slabs on disk
                uint64_t _nbSlabs;

        public:

                /**
                 * The maximum number of slabs that can be stored in the collection
                 */
                MetaDiskCollection (uint32_t maxNbSlabs);

                /**
                 * Create a new slab from a memory slab
                 * @params:
                 *    - slab: the memory slab
                 *    - hash: the list of hashs that can be found in the slab
                 * @returns: the id of the created slab
                 */
                void createSlabFromRAM (const memory::KVMapRAMSlab & slab, const std::set <uint64_t> & hash);

                /**
                 * Insert a key value in the collection
                 */
                void insert (const common::Key & k, const common::Value & v);

                /**
                 * Find the key in the collection
                 */
                std::shared_ptr <common::Value> find (const common::Key & k);

                /**
                 * @returns: the maximum number of slabs on disk
                 */
                uint32_t getMaxSlabs () const;

                /**
                 * Remove the key in the collection
                 * @info: does nothing if the key is not found
                 */
                void remove (const common::Key & k);

                /**
                 * Clear all the slabs from disk
                 */
                void dispose ();

                /**
                 * this-> dispose ()
                 */
                ~MetaDiskCollection ();

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ======================================          MISC          ======================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                friend std::ostream & operator<< (std::ostream & s, const MetaDiskCollection & mp);

        private:

                /**
                 * Find and erase one slab on disk
                 */
                void findAndEraseSlab ();

        };

}
