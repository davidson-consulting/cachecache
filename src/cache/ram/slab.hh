#pragma once

#include <cstdint>
#include <memory>
#include <iostream>

#include <rd_utils/utils/mem_size.hh>

#include "lru.hh"
#include "freelist.hh"
#include "../common/csts.hh"
#include "../common/key.hh"
#include "../common/value.hh"

namespace kv_store::memory {
    class MetaRamCollection;
    class TTLMetaRamCollection;
    class WSSMetaRamCollection;


    class KVMapRAMSlab {
    private:

        struct node {
            uint32_t keySize;
            uint32_t valueSize;
            uint32_t next;
        };

        struct instance {
            uint32_t len;
            uint32_t lst [kv_store::common::NB_KVMAP_SLAB_ENTRIES];
        };

    private:

        // The global uniq id
        static uint32_t __ID__;

        // The uniq Id of the slab
        uint32_t _uniqId;

        // The free list used to allocate
        FreeList _context;

    public:

        class Iterator {
        private:

            const KVMapRAMSlab * context;
            uint32_t currListIndex;
            uint32_t currListOffset;
            uint32_t currentIndex;

        public:

            Iterator ();
            Iterator (const KVMapRAMSlab * context, uint32_t baseIndex, uint32_t listOffset);

            std::pair <std::shared_ptr <common::Key>, std::shared_ptr <common::Value> > operator* () const;
            Iterator& operator++ ();

            friend bool operator== (const Iterator & a, const Iterator & b);
            friend bool operator!= (const Iterator & a, const Iterator & b);

        };

    public:

        friend MetaRamCollection;
        friend TTLMetaRamCollection;
        friend WSSMetaRamCollection;

    public:

        /**
         * Load a slab from memory segment (RAM)
         * @params:
         *    - memory: the memory content of the slab
         *    - id: the uniq id of the slab
         */
        KVMapRAMSlab (uint8_t * memory, uint32_t id);

        /**
         * Allocate a new slab in memory (RAM)
         */
        KVMapRAMSlab ();

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * =================================          INSERT/FIND/RM          =================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Insert a key/value in the slab
         * @params:
         *    - key: the key
         *    - value: the value to write in the slab
         * @returns: true iif the insertion was successful
         */
        bool insert (const common::Key & key, const common::Value & value);

        /**
         * Find a value using a key in the slab and return a copy of it
         * @params:
         *    - key: the key to find
         * @returns: the value (nullptr if not found)
         */
        std::shared_ptr <common::Value> find (const common::Key & key) const;

        /**
         * Remove a key from the memory segment in the slab
         * @info: does nothing if the key is not found
         * @returns: true if memory was freed, false if nothing was changed
         */
        bool remove (const common::Key & key);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ==================================          DIRECT GETS          ===================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Get a KV in the store from its direct offset
         */
        void getFromOffset (uint32_t offset, std::shared_ptr <common::Key> & k, std::shared_ptr <common::Value> & v);

        /**
         * @returns: the free list memory segment
         */
        const FreeList & getMemory () const;

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
        ~KVMapRAMSlab ();

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
        KVMapRAMSlab::Iterator begin () const;

        /**
         * End iterator to iterate over the key/value in the map
         */
        KVMapRAMSlab::Iterator end () const;

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

        /**
         * @returns: the uniq id of the slab
         */
        uint32_t getUniqId () const;

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ======================================          MISC          ======================================
         * ====================================================================================================
         * ====================================================================================================
         */

        friend std::ostream & operator<< (std::ostream & s, const KVMapRAMSlab & mp);

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
         * @returns: the offset of the insertion (0 if failed)
         */
        bool insertAfter (const common::Key & k, const common::Value & v, node * n);

        /**
         * Create a new entry to store the key/value
         * @params:
         *    - k: the key of the entry
         *    - v: the value to put in the entry
         *    - prevPtr: the offset pointer in the chained list to update
         */
        bool createNewEntry (const common::Key & k, const common::Value & v, uint32_t & prevPtr);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          FINDING          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        std::shared_ptr <common::Value> findInList (const common::Key & k, const node* n) const;

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
        bool removeInList (const common::Key & k, node * n, uint32_t & prevPtr);

    };

}
