#pragma once

#include "freelist.hh"

namespace kv_store::memory {

    /**
     * LRU linked list implemented in a slab
     */
    class SlabLRU {

        // The context of the allocation
        FreeList _context;

    public :

        struct LRUInfo {
            uint32_t slabId;
            uint32_t offset;
            uint32_t next;
            uint32_t prev;
        };

        uint32_t * _head;

    public:

        /**
         * Create a slab LRU
         */
        SlabLRU ();

        /**
         * Insert a LRU node in the slab
         * @params:
         *    - slabId: the slab containing the key
         *    - offset: the offset of the key in the slab
         * @returns: the offset of the allocated LRU node
         */
        uint32_t insert (uint32_t slabId, uint32_t offset);

        /**
         * Push the lru node as the front node (last accessed value)
         */
        void pushFront (uint32_t lruOffset);

        /**
         * @returns: the oldest node in the LRU
         */
        bool getOldest (LRUInfo & info) const;

        /**
         * Remove a LRU node from the listo
         */
        void erase (uint32_t lruOffset);

        /**
         * Free everything
         */
        void dispose ();

        /**
         * this-> dispose ();
         */
        ~SlabLRU ();

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ======================================          MISC          ======================================
         * ====================================================================================================
         * ====================================================================================================
         */

        friend std::ostream & operator<< (std::ostream & s, const SlabLRU & lru);

    };


}
