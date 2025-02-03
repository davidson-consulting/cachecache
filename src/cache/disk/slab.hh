#pragma once

#include <cstdint>
#include <memory>
#include <iostream>

#include <rd_utils/utils/mem_size.hh>

#include "freelist.hh"
#include "../common/csts.hh"
#include "../common/key.hh"
#include "../common/value.hh"

namespace kv_store::memory {
    class KVMapRAMSlab;
}

namespace kv_store::disk {

    class MetaDiskCollection;
    class KVMapDiskSlab {
    private:

        static constexpr uint32_t __HEAD_OFFSET__ = 2 * sizeof (uint32_t);
        static constexpr uint32_t __LEN_OFFSET__  = sizeof (uint32_t);

    private:

        struct node {
            uint32_t keySize;
            uint32_t valueSize;
            uint32_t next;
        };

    private:

        // The global uniq id
        static uint32_t __ID__;

        // The uniq id of the slab
        uint32_t _uniqId;

        // The freelist used to allocate
        FreeList _context;

    public:

        class Iterator {
        private:

            const KVMapDiskSlab * context;
            uint32_t currListIndex;
            uint32_t currListOffset;
            uint32_t currentIndex;

        public:

            Iterator ();
            Iterator (const KVMapDiskSlab * context, uint32_t baseIndex, uint32_t listOffset);

            std::pair <std::shared_ptr <common::Key>, std::shared_ptr <common::Value> > operator* () const;
            Iterator& operator++ ();

            friend bool operator== (Iterator & a, Iterator & b);
            friend bool operator!= (Iterator & a, Iterator & b);

        };

    public:

        friend MetaDiskCollection;

    public:

        /**
         * Load a slab from disk
         * @params:
         *   - id: the id of the slab to load
         */
        KVMapDiskSlab (uint32_t id);

        /**
         * Create a new slab on disk
         */
        KVMapDiskSlab ();

        /**
         * Copy a memory slab to disk
         */
        KVMapDiskSlab (const memory::KVMapRAMSlab & mem);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===============================          INSERT/LOAD/REMOVE          ===============================
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
         * ===================================          DISPOSING          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Close all handles
         * @info: does not remove the slab from the disk
         */
        void dispose ();

        /**
         * Erase the slab from the disk
         */
        void erase ();

        /**
         * this-> dispose ();
         */
        ~KVMapDiskSlab ();


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
        KVMapDiskSlab::Iterator begin () const;

        /**
         * End iterator to iterate over the key/value in the map
         */
        KVMapDiskSlab::Iterator end () const;

        /**
         * @returns: the number of elements in the slab
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

        friend std::ostream & operator<< (std::ostream & s, const KVMapDiskSlab & mp);

    private:

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ===================================          CONFIGURE          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Initialize the key value store in the slab
         */
        void init ();

        /**
         * Load the configuration of the k/v store
         */
        void load ();

        /**
         * Init the slab from memory slab
         */
        void initFromMemory (uint32_t size, const uint8_t* data);

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
         *    - offset: the offset of the node in the chained list
         */
        bool insertAfter (const common::Key & k, const common::Value & v, uint32_t offset);


        /**
         * Create a new entry to store the key/value
         * @params:
         *    - k: the key of the entry
         *    - v: the value to put in the entry
         *    - prevPtr: the offset pointer in the chained list to update
         */
        bool createNewEntry (const common::Key & k, const common::Value & v, uint32_t & offset);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          FINDING          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        std::shared_ptr <common::Value> findInList (const common::Key & k, uint32_t offset) const;

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
        bool removeInList (const common::Key & k, uint32_t offset, uint32_t prevOffset);

    };


}
