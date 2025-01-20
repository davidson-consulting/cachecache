#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include "slab.hh"
#include "map.hh"

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
