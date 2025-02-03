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

    public:

        /**
         * The maximum number of slabs that can be stored in the collection
         */
        MetaDiskCollection ();

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
         * Remove the key in the collection
         * @info: does nothing if the key is not found
         */
        void remove (const common::Key & k);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ======================================          MISC          ======================================
         * ====================================================================================================
         * ====================================================================================================
         */

        friend std::ostream & operator<< (std::ostream & s, const MetaDiskCollection & mp);

    };

}
