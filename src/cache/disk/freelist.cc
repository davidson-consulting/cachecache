#include "freelist.hh"
#include "../common/csts.hh"
#include <rd_utils/utils/files.hh>
#include <string.h>

namespace kv_store::disk {

    FreeList::FreeList (uint32_t id, uint32_t size)
        : _id (id)
        , _handle (nullptr)
        , _size (size)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void FreeList::load () {
        auto path = std::string (common::KVMAP_DISK_PATH) + std::to_string (this-> _id);
        if (!rd_utils::utils::file_exists (path)) {
            this-> init ();
            return;
        }

        this-> _handle = fopen (path.c_str (), "rb+");
        if (this-> _handle == nullptr) {
            throw std::runtime_error ("Failed to open freelist file : " + path);
        }
    }

    void FreeList::init () {
        auto path = std::string (common::KVMAP_DISK_PATH) + std::to_string (this-> _id);
        if (!rd_utils::utils::directory_exists (common::KVMAP_DISK_PATH)) {
            rd_utils::utils::create_directory (common::KVMAP_DISK_PATH, true);
        }

        this-> _handle = fopen (path.c_str (), "wb+");
        if (this-> _handle == nullptr) {
            throw std::runtime_error ("Failed to open freelist file : " + path);
        }

        uint8_t * buffer = new uint8_t [1024];
        uint32_t nbPass = this-> _size / 1024;
        uint32_t remain = this-> _size % 1024;

        fseek (this-> _handle, 0, SEEK_SET);
        for (uint32_t i = 0 ; i < nbPass ; i++) {
            if (fwrite (buffer, sizeof (uint8_t), 1024, this-> _handle) != 1024) {
                throw std::runtime_error (std::string ("Failed to init freelist file : ") + strerror (errno));
            }
        }

        if (remain != 0) {
            if (fwrite (buffer, sizeof (uint8_t), remain, this-> _handle) != remain) {
                throw std::runtime_error (std::string ("Failed to init freelist file : ") + strerror (errno));
            }
        }

        delete [] buffer;

        // list-> size
        fseek (this-> _handle, 0, SEEK_SET);
        fwrite (&this-> _size, sizeof (uint32_t), 1, this-> _handle);

        // list-> head
        uint32_t len = sizeof (instance);
        fseek (this-> _handle, sizeof (uint32_t), SEEK_SET);
        fwrite (&len, sizeof (uint32_t), 1, this-> _handle);

        // head-> size
        len = this-> _size - sizeof (instance);
        fseek (this-> _handle, sizeof (instance), SEEK_SET);
        fwrite (&len, sizeof (uint32_t), 1, this-> _handle);

        // head-> next is already equal to 0
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          DISPOSE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void FreeList::dispose () {
        if (this-> _handle != nullptr) {
            fclose (this-> _handle);
            this-> _handle = nullptr;
        }
    }

    void FreeList::erase () {
        this-> dispose ();
        auto path = std::string (common::KVMAP_DISK_PATH) + std::to_string (this-> _id);
        rd_utils::utils::remove (path);
    }

    FreeList::~FreeList () {
        this-> dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          ALLOC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t FreeList::alloc (uint32_t size) {
        uint32_t reqSize = this-> realAllocSize (size);
        uint32_t offset = 0;

        if (this-> performAlloc (reqSize, offset)) {
            this-> writeIntAt (offset, reqSize);
            return offset + sizeof (uint32_t);
        }

        return 0;
    }

    bool FreeList::free (uint32_t offset) {
        uint32_t len = this-> readIntAt (offset - sizeof (uint32_t));
        return this-> performFree (offset - sizeof (uint32_t), len);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t FreeList::realAllocSize (uint32_t size) {
        uint32_t reqSize = size + sizeof (uint32_t);
        if (reqSize < sizeof (node)) {
            return sizeof (node);
        }

        return reqSize;
    }

    bool FreeList::isEmpty () const {
        uint32_t head = 0, size = 0;
        fseek (this-> _handle, 0, SEEK_SET);
        fread (&size, sizeof (uint32_t), 1, this-> _handle);
        fread (&head, sizeof (uint32_t), 1, this-> _handle);

        if (head == sizeof (instance)) {
            fread (&head, sizeof (uint32_t), 1, this-> _handle);
            return head == size;
        }

        return false;
    }

    uint32_t FreeList::maxAllocSize () const {
        uint32_t nodeOffset = this-> readIntAt (sizeof (uint32_t));
        uint32_t len = 0, next = 0;
        uint32_t max = 0;
        while (nodeOffset != 0) {
            fseek (this-> _handle, nodeOffset, SEEK_SET);
            fread (&len, sizeof (uint32_t), 1, this-> _handle);
            fread (&next, sizeof (uint32_t), 1, this-> _handle);

            if (len > max) {
                max = len;
            }

            nodeOffset = next;
        }

        if (max > sizeof (uint32_t)) {
            return max - sizeof (uint32_t);
        }

        return 0;
    }

    uint32_t FreeList::remainingSize () const {
        uint32_t nodeOffset = this-> readIntAt (sizeof (uint32_t));
        uint32_t len = 0, next = 0;
        uint32_t sum = 0;
        while (nodeOffset != 0) {
            fseek (this-> _handle, nodeOffset, SEEK_SET);
            fread (&len, sizeof (uint32_t), 1, this-> _handle);
            fread (&next, sizeof (uint32_t), 1, this-> _handle);

            sum += (len - sizeof (uint32_t));
            nodeOffset = next;
        }

        return sum;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          PRIVATE ALLOC          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool FreeList::findBestFit (uint32_t size, uint32_t & offset, uint32_t & prevOffset, uint32_t & nodeSize, uint32_t & nodeNext) const {
        uint32_t nodeOffset = this-> readIntAt (sizeof (uint32_t));
        uint32_t prev = 0;
        uint32_t minFound = this-> _size + 1;
        bool found = false;
        uint32_t len = 0, next = 0;

        while (nodeOffset != 0) {
            fseek (this-> _handle, nodeOffset, SEEK_SET);
            fread (&len, sizeof (uint32_t), 1, this-> _handle);
            fread (&next, sizeof (uint32_t), 1, this-> _handle);

            if (len >= size && (minFound > len || !found)) {
                offset = nodeOffset;
                prevOffset = prev;
                minFound = len;
                nodeSize = len;
                nodeNext = next;
                found = true;
            }

            prev = nodeOffset;
            nodeOffset = next;
        }

        return found;
    }

    bool FreeList::performAlloc (uint32_t & size, uint32_t & offset) {
        uint32_t prevOffset = 0, nodeOffset = 0, nodeSize = 0, nodeNext = 0;
        if (!this-> findBestFit (size, nodeOffset, prevOffset, nodeSize, nodeNext)) {
            return false;
        }

        offset = nodeOffset;

        uint32_t restSize = nodeSize - size;
        if (restSize >= 2 * sizeof (uint32_t)) { // create a new node
            uint32_t newNodeOffset = nodeOffset + size;
            fseek (this-> _handle, newNodeOffset, SEEK_SET);
            fwrite (&restSize, sizeof (uint32_t), 1, this-> _handle);
            fwrite (&nodeNext, sizeof (uint32_t), 1, this-> _handle);

            if (prevOffset != 0) {
                this-> writeIntAt (prevOffset + sizeof (uint32_t), newNodeOffset);
            } else {
                this-> writeIntAt (sizeof (uint32_t), newNodeOffset);
            }
        } else { // not enough size for a new node, use it all
            size = nodeSize;

            if (prevOffset != 0) {
                this-> writeIntAt (prevOffset + sizeof (uint32_t), nodeNext);
            } else {
                this-> writeIntAt (sizeof (uint32_t), nodeNext);
            }
        }

        return true;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ==================================          PRIVATE FREE          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool FreeList::performFree (uint32_t offset, uint32_t size) {
        uint32_t nodeOffset = this-> readIntAt (sizeof (uint32_t));
        uint32_t prevOffset = 0, prevSize = 0;

        if (nodeOffset == 0) {
            this-> writeIntAt (sizeof (uint32_t), offset);
            fseek (this-> _handle, offset, SEEK_SET);
            fwrite (&size, sizeof (uint32_t), 1, this-> _handle);
            uint32_t zero = 0;
            fwrite (&zero, sizeof (uint32_t), 1, this-> _handle);

            return true;
        }

        uint32_t nodeSize = 0, nodeNext = 0;
        while (nodeOffset != 0) {
            fseek (this-> _handle, nodeOffset, SEEK_SET);
            fread (&nodeSize, sizeof (uint32_t), 1, this-> _handle);
            fread (&nodeNext, sizeof (uint32_t), 1, this-> _handle);

            if (nodeOffset + nodeSize == offset) {
                if (nodeNext == nodeOffset + (nodeSize + size)) {
                    uint32_t nextSize = 0, nextNext = 0;
                    fseek (this-> _handle, nodeNext, SEEK_SET);
                    fread (&nextSize, sizeof (uint32_t), 1, this-> _handle);
                    fread (&nextNext, sizeof (uint32_t), 1, this-> _handle);

                    nodeSize = nodeSize + size + nextSize;
                    fseek (this-> _handle, nodeOffset, SEEK_SET);
                    fwrite (&nodeSize, sizeof (uint32_t), 1, this-> _handle);
                    fwrite (&nextNext, sizeof (uint32_t), 1, this-> _handle);
                } else {
                    nodeSize = nodeSize + size;
                    fseek (this-> _handle, nodeOffset, SEEK_SET);
                    fwrite (&nodeSize, sizeof (uint32_t), 1, this-> _handle);
                }

                return true;
            }


            if (nodeOffset > offset) { // we past the offset of the block to free
                uint32_t newNodeOffset = offset;
                bool touchPrev = (prevOffset != 0 && prevOffset + prevSize == offset);
                bool touchNext = (newNodeOffset + size == nodeOffset);

                if (touchNext && touchPrev) { // fusion the three nodes
                    uint32_t nextSize = 0, nextNext = 0;
                    fseek (this-> _handle, nodeNext, SEEK_SET);
                    fread (&nextSize, sizeof (uint32_t), 1, this-> _handle);
                    fread (&nextNext, sizeof (uint32_t), 1, this-> _handle);

                    fseek (this-> _handle, prevOffset, SEEK_SET);
                    prevSize = prevSize + size + nextSize;
                    fwrite (&prevSize, sizeof (uint32_t), 1, this-> _handle);
                    fwrite (&nextNext, sizeof (uint32_t), 1, this-> _handle);
                }

                else if (touchPrev) { // fusion prev and new node
                    prevSize = prevSize + size;
                    fseek (this-> _handle, prevOffset, SEEK_SET);
                    fwrite (&prevSize, sizeof (uint32_t), 1, this-> _handle);
                }

                else if (touchNext) { // fusion new node and next
                    fseek (this-> _handle, newNodeOffset, SEEK_SET);
                    nodeSize = size + nodeSize;
                    fwrite (&nodeSize, sizeof (uint32_t), 1, this-> _handle);
                    fwrite (&nodeNext, sizeof (uint32_t), 1, this-> _handle);

                    fseek (this-> _handle, prevOffset + sizeof (uint32_t), SEEK_SET);
                    fwrite (&newNodeOffset, sizeof (uint32_t), 1, this-> _handle);
                }
                
                else { // insert a new node
                    fseek (this-> _handle, newNodeOffset, SEEK_SET);
                    fwrite (&size, sizeof (uint32_t), 1, this-> _handle);
                    fwrite (&nodeOffset, sizeof (uint32_t), 1, this-> _handle);

                    fseek (this-> _handle, prevOffset + sizeof (uint32_t), SEEK_SET);
                    fwrite (&newNodeOffset, sizeof (uint32_t), 1, this-> _handle);
                }

                return true;
            }

            // Last node but the offset of after it
            if (nodeNext == 0 && nodeOffset + nodeSize < offset) {
                fseek (this-> _handle, offset, SEEK_SET);
                fwrite (&size, sizeof (uint32_t), 1, this-> _handle);

                uint32_t zero = 0;
                fwrite (&zero, sizeof (uint32_t), 1, this-> _handle);

                // prev-> next = newNode
                fseek (this-> _handle, nodeOffset + sizeof (uint32_t), SEEK_SET);
                fwrite (&offset, sizeof (uint32_t), 1, this-> _handle);

                return true;
            }


            prevOffset = nodeOffset;
            prevSize = nodeSize;
            nodeOffset = nodeNext;
        }

        return false;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          WRITE/READ          ===================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void FreeList::write (uint32_t offset, const uint8_t* data, uint32_t len) {
        fseek (this-> _handle, offset, SEEK_SET);
        fwrite (data, sizeof (uint8_t), len, this-> _handle);
    }

    void FreeList::set (uint32_t offset, uint32_t value, uint32_t len) {
        fseek (this-> _handle, offset, SEEK_SET);
        for (uint32_t i = 0 ; i < len / sizeof (uint32_t) ; i++) {
            fwrite (&value, sizeof (uint8_t), sizeof (uint32_t), this-> _handle);
        }

        uint32_t rest = len % sizeof (uint32_t);
        if (rest != 0) {
            fwrite (&value, sizeof (uint8_t), rest, this-> _handle);
        }
    }

    void FreeList::read (uint32_t offset, uint8_t * data, uint32_t len) const {
        fseek (this-> _handle, offset, SEEK_SET);
        fread (data, sizeof (uint8_t), len, this-> _handle);
    }

    bool FreeList::compare (uint32_t offset, const uint8_t * data, uint32_t len) const {
        fseek (this-> _handle, offset, SEEK_SET);
        char c;
        for (uint32_t i = 0 ; i < len ; i++) {
            fread (&c, sizeof (uint8_t), 1, this-> _handle);
            if (c != data [i]) return false;
        }

        return true;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          PRIVATE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t FreeList::readIntAt (uint32_t offset) const {
        fseek (this-> _handle, offset, SEEK_SET);
        uint32_t result = 0;
        fread (&result, sizeof (uint32_t), 1, this-> _handle);
        return result;
    }

    void FreeList::writeIntAt (uint32_t offset, uint32_t value) {
        fseek (this-> _handle, offset, SEEK_SET);
        fwrite (&value, sizeof (uint32_t), 1, this-> _handle);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::ostream & operator<< (std::ostream & s, const FreeList & lst) {
        s << "LIST{";
        uint32_t nodeOffset = lst.readIntAt (sizeof (uint32_t));
        uint32_t len = 0, next = 0;
        while (nodeOffset != 0) {
            fseek (lst._handle, nodeOffset, SEEK_SET);
            fread (&len, sizeof (uint32_t), 1, lst._handle);
            fread (&next, sizeof (uint32_t), 1, lst._handle);

            s << "(" << nodeOffset << "," << len << ")";
            nodeOffset = next;
        }

        s << "}";
        return s;
    }

}
