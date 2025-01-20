#pragma once

#include <iostream>
#include <cstdint>
#include <cstdio>
#include <string>

namespace kv_store::disk {

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
                uint32_t _id;

                // the handle to the segment file
                FILE * _handle;

                // The size of the segment
                uint32_t _size;

        public:

                /**
                 *
                 */
                FreeList (uint32_t id, uint32_t totalSize);

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ===================================          CONFIGURE          ====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Load the free list from the content of the file
                 */
                void load ();

                /**
                 * Initialize the freelist memory segment and write the default layout to the file
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
                 * ===================================          WRITE/READ          ===================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Write data to the file segment
                 * @params:
                 *    - offset: the offset of the writing
                 *    - data: the data to write
                 *    - len: the size of the data to write
                 */
                void write (uint32_t offset, const uint8_t * data, uint32_t len);

                /**
                 * Set bytes to a certain value
                 * @params:
                 *    - offset: the offset of the first byte to set
                 *    - value: the value to set
                 *    - len: the number of bytes to set
                 */
                void set (uint32_t offset, uint32_t value, uint32_t len);

                /**
                 * Read data from the file segment
                 * @params:
                 *    - offset: the offset of the reading
                 *    - data: the data to read
                 *    - len: the length to read
                 */
                void read (uint32_t offset, uint8_t * data, uint32_t len) const;

                /**
                 * Compare disk content with a segment of memory
                 */
                bool compare (uint32_t offset, const uint8_t * data, uint32_t len) const;

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ===================================          DISPOSING          ====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Close the handle (but does not remove the file)
                 */
                void dispose ();

                /**
                 * Close the handle and remove the file
                 */
                void erase ();

                /**
                 * this-> dispose ();
                 */
                ~FreeList ();

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
                 *    - nodeSize: the size of the free node
                 *    - nodeNext: the next element in the list
                 */
                bool findBestFit (uint32_t size, uint32_t & offset, uint32_t & prevOffset, uint32_t & nodeSize, uint32_t & nodeNext) const;

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

                /**
                 * Read an int at offset
                 */
                uint32_t readIntAt (uint32_t offset) const;

                /**
                 * write an int at offset
                 */
                void writeIntAt (uint32_t offset, uint32_t value);

        };

}
