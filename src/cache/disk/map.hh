#pragma once

#include <cstdint>
#include <memory>
#include <iostream>

#include <rd_utils/utils/mem_size.hh>

#include "freelist.hh"
#include "../common/csts.hh"
#include "../common/key.hh"
#include "../common/value.hh"

namespace kv_store::disk {

    class MetaDiskCollection;

    /**
     * Class used to store the list of slab associations and avoid reading all
     * slab to find a memory location
     */
    class DiskMap {

        static constexpr uint32_t __HEAD_OFFSET__ = 4 * sizeof (uint32_t);

        struct node {
            uint32_t slabId;
            uint32_t nbs;
            uint32_t next;
        };

        // The global uniq id
        static uint32_t __ID__;

        // The uniq id of the slab
        uint32_t _uniqId;

        // The freelist used to allocate
        FreeList _context;

    public:

        class SlabListIterator {
        private:

            const DiskMap * context;
            uint32_t currListOffset;

        public:

            SlabListIterator ();
            SlabListIterator (const DiskMap * context, uint32_t listOffset);

            std::pair <uint32_t, uint32_t> operator* () const;
            SlabListIterator& operator++ ();

            friend bool operator== (const SlabListIterator & a, const SlabListIterator & b);
            friend bool operator!= (const SlabListIterator & a, const SlabListIterator & b);

            bool isEnd () const;
        };

    public:

        friend MetaDiskCollection;

    public:


        /**
         * Load the meta information from disk
         */
        DiskMap (uint32_t id);

        /**
         * Create a new meta infor on the disk
         */
        DiskMap ();


        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===============================          INSERT/LOAD/REMOVE          ===============================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Insert a slab id in the list
         */
        void insert (uint64_t h, uint32_t slabId);

        /**
         * Remove a slab id in the list
         */
        void remove (uint64_t h, uint32_t slabId);

        /**
         * @returns: the list of slabs containing a key whose hash is 'h'
         */
        SlabListIterator find (uint64_t h) const;

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===================================          DISPOSING          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Dispose the meta information
         * @info: does not remove the file from disk
         */
        void dispose () ;

        /**
         * Erase the meta information from the disk
         */
        void erase ();

        /**
         * this-> dispose ();
         */
        ~DiskMap ();

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===================================          ITERATION          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * @returns: the size of the biggest free block to store a k/v
         */
        rd_utils::utils::MemorySize maxAllocSize () const;

        /**
         * @returns: the sum of remaining free memory size
         */
        rd_utils::utils::MemorySize remainingSize () const;

        /**
         * @returns: the uniq id of the slab
         */
        uint32_t getUniqId () const;

    private:

        /**
         * Initialize the map in the free list
         */
        void init ();

        /**
         * Load the map from disk
         */
        void load ();


        /*!
         * ====================================================================================================
         * ====================================================================================================
         * =====================================          INSERT          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Insert the element in the free list
         */
        void insertAfter (uint32_t slab, uint32_t offset);

        /**
         * Create a new node at the end
         */
        void createNewEntry (uint32_t v, uint32_t & result);


        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          REMOVING          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Decrement the counter of the slab in the list
         */
        void removeInList (uint32_t slabId, uint32_t offset, uint32_t prev);

    };

}
