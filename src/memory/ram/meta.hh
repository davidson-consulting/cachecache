#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include "../slab.hh"

namespace kv_store::memory {

    /**
     * The meta ram collection stores the informations of the keys stored in the RAM
     */
    class MetaRamCollection {

        // The number of slabs that can be managed by the collection
        uint32_t _maxNbSlabs;

        // The RAM slabs storing KVs
        std::map <uint32_t, std::shared_ptr <KVMapSlab> > _loadedSlabs;

        // The list of slabs containing a key
        std::map <uint64_t, std::map <uint32_t, uint32_t> > _slabHeads;

    public:

        /**
         * The maximum number of slabs that can be stored in the collection
         */
        MetaRamCollection (uint32_t maxNbSlabs);

        /**
         * Insert a key value in the collection
         */
        void insert (const Key & k, const Value & v);

        /**
         * Find the key in the collection
         */
        std::shared_ptr <Value> find (const Key & k);

        /**
         * Remove the key in the collection
         * @info: does nothing if the key is not found
         */
        void remove (const Key & k);

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
