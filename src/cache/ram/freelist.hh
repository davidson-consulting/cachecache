#pragma once

#include <cstdint>
#include <iostream>

namespace kv_store::memory {

    class FreeList {
    private:

        struct node {
            uint32_t size;
            uint32_t next;
        };

        struct instance {
            uint32_t size;
            uint32_t head;
        };

        // The memory managed by the free list
        uint8_t * _memory;
        uint32_t _size;

    public:

        /**
         * Calc the behavior of a freelist in the memory segment (previously allocated)
         * @warning: or the segment already contains a free list, or it need to be initialized by this-> init
         * @params:
         *    - memory: the memory segment manged as a free list
         *    - size: the size of the memory segment available on free list
         */
        FreeList (uint8_t * memory, uint32_t totalSize);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===================================          CONFIGURE          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Load the free list from the segment (previously initialized)
         */
        void load ();

        /**
         * Initialize the freelist memory segment by clearing everything and setting default layout
         */
        void init ();

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * =====================================          ALLOC          ======================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Tries to find a segment in which the size fits
         * @returns: the offset of the segment, (0 if not found)
         */
        uint32_t alloc (uint32_t size);

        /**
         * Free the segment of memory at offset
         * @params:
         *    - offset: the offset to free
         */
        bool free (uint32_t offset);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GETTERS          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * @returns: the pointer to the start of the segment managed by the freelist (after free list metadata)
         */
        uint8_t* data ();

        /**
         * @returns: the pointer to the start of the segment managed by the freelist (after free list metadata)
         */
        const uint8_t* data () const;

        /**
         * @returns: the pointer to the start of the segment managed by the freelist (including free list metadata)
         */
        uint8_t* metadata ();

        /**
         * @returns: the size that will be allocated to store an object of size size
         */
        uint32_t realAllocSize (uint32_t size);

        /**
         * @returns: true if the list is empty
         */
        bool isEmpty () const;

        /**
         * @returns: the size of the biggest free block in the list
         */
        uint32_t maxAllocSize () const;

        /**
         * @returns: the sum of remaining size
         */
        uint32_t remainingSize () const;

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ======================================          MISC          ======================================
         * ====================================================================================================
         * ====================================================================================================
         */

        friend std::ostream & operator<< (std::ostream & os, const FreeList & inst);

    private:

        /**
         * Best fit allocation to find the memory segment whose size is the closest to 'size'
         * @params:
         *    - size: the size we are trying to allocate
         * @returns:
         *    - true iif found
         *    - offset: the returned offset
         *    - prevOffset: the offset of the node preceding the available node (to be updated)
         */
        bool findBestFit (uint32_t size, uint32_t & offset, uint32_t & prevOffset) const;

        /**
         * Perform the allocation (finding and using)
         * @params:
         *    - size: the size to allocate (without metadata)
         * @returns:
         *    - true iif found
         *    - size: the real size of the allocation
         *    - offset: the result offset
         */
        bool performAlloc (uint32_t & size, uint32_t & offset);

        /**
         * Perform the free (finding and releasing)
         * @params:
         *    - offset: the offset to free
         *    - size: the size of the allocation to free
         * @returns:
         *    - true iif the node was found and freed
         */
        bool performFree (uint32_t offset, uint32_t size);

    };

}
