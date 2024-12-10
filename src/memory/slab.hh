#pragma once

#include <cstdint>
#include <memory>
#include <iostream>

#include <rd_utils/utils/mem_size.hh>

#include "utils/csts.hh"
#include "utils/free_list.hh"
#include "key.hh"
#include "value.hh"


namespace kv_store::memory {

    class KVMapSlab {
    private:

        struct node {
            uint32_t keySize;
            uint32_t valueSize;
            uint32_t next;
        };

        struct instance {
            uint32_t lst [kv_store::memory::NB_KVMAP_SLAB_ENTRIES];
            uint32_t len;
        };

    private:

        // The uniq Id of the slab
        uint32_t _uniqId;

        // The free list used to allocate
        FreeList _context;

    public:

        class Iterator {
        private:

            const KVMapSlab * context;
            uint32_t currListIndex;
            uint32_t currListOffset;
            uint32_t currentIndex;

        public:

            Iterator ();
            Iterator (const KVMapSlab * context, uint32_t baseIndex, uint32_t listOffset);

            std::pair <std::shared_ptr <Key>, std::shared_ptr <Value> > operator* () const;
            Iterator& operator++ ();

            friend bool operator== (Iterator & a, Iterator & b);
            friend bool operator!= (Iterator & a, Iterator & b);

        };


    public:

        /**
         * Load a slab from memory segment (RAM)
         * @params:
         *    - memory: the memory content of the slab
         *    - id: the uniq id of the slab
         */
        KVMapSlab (uint8_t * memory, uint32_t id);

        /**
         * Allocate a new slab in memory (RAM)
         * @params:
         *    - id: the uniq id of the slab
         */
        KVMapSlab (uint32_t id);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * =================================          INSERT/FIND/RM          =================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Perform an allocation in the slab and returns the pointer to the memory segment
         * @params:
         *    - key: the key
         *    - value: the value to write in the slab
         * @returns: true iif the allocation was successful
         */
        bool alloc (const Key & key, const Value & value);

        /**
         * Find an allocation in the slab and return a copy of it
         * @params:
         *    - key: the key to find
         * @returns: the value (nullptr if not found)
         */
        std::shared_ptr <Value> find (const Key & key) const;

        /**
         * Remove a key from the memory segment in the slab
         * @info: does nothing if the key is not found
         */
        void remove (const Key & key);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===================================          DISPOSING          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Dispose the slab memory segment
         */
        void dispose ();

        /**
         * this-> dispose ();
         */
        ~KVMapSlab ();

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===================================          ITERATION          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Begin iterator to iterate over the key/value in the map
         */
        KVMapSlab::Iterator begin () const;

        /**
         * End iterator to iterate over the key/value in the map
         */
        KVMapSlab::Iterator end () const;

        /**
         * @returns: the number of elements in the map
         */
        uint32_t len () const;

        /**
         * @returns: the size of the biggest free block to store a k/v
         */
        rd_utils::utils::MemorySize maxAllocSize () const;

        /**
         * @returns: the sum of remaining free memory size
         */
        rd_utils::utils::MemorySize remainingSize () const;

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ======================================          MISC          ======================================
         * ====================================================================================================
         * ====================================================================================================
         */

        friend std::ostream & operator<< (std::ostream & s, const KVMapSlab & mp);

    private:

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===================================          CONFIGURE          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Initialize the key/value store in the slab
         */
        void init ();

        /**
         * Load the configuration of the key/value store (where current memory allocation was already prev configured)
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
         * Insert the key at the end of the list
         * @params:
         *    - k: the key to insert
         *    - v: the value to insert
         *    - n: the node in the chained list
         */
        bool insertAfter (const Key & k, const Value & v, node * n);

        /**
         * Create a new entry to store the key/value
         * @params:
         *    - k: the key of the entry
         *    - v: the value to put in the entry
         *    - prevPtr: the offset pointer in the chained list to update
         */
        bool createNewEntry (const Key & k, const Value & v, uint32_t & prevPtr);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          FINDING          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        std::shared_ptr <Value> findInList (const Key & k, const node* n) const;

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          REMOVING          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Remove an entry in the list
         */
        bool removeInList (const Key & k, node * n, uint32_t & prevPtr);

    };

}
