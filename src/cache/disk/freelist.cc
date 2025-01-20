#include "freelist.hh"
#include "../common/csts.hh"
#include <rd_utils/utils/files.hh>
#include <string.h>
#include <unistd.h>

namespace kv_store::disk {

    FreeList::FreeList (uint32_t id)
        : _id (id)
        , _handle (nullptr)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void FreeList::load (const std::string & slabPath) {
        auto path = slabPath + std::to_string (this-> _id);
        this-> _handle = fopen (path.c_str (), "rb+");
        if (this-> _handle == nullptr) {
            throw std::runtime_error ("Failed to open freelist file : " + path);
        }
    }

    void FreeList::init (uint32_t size, const std::string & slabPath) {
        auto path = slabPath + std::to_string (this-> _id);
        this-> _handle = fopen (path.c_str (), "wb+");
        if (this-> _handle == nullptr) {
            throw std::runtime_error ("Failed to open freelist file : " + path);
        }

        // resize the file to the correct length
        ftruncate (fileno (this-> _handle), size);

        // list-> size
        this-> write <uint32_t> (__SIZE_OFFSET__, size);

        // list-> head
        this-> write <uint32_t> (__HEAD_OFFSET__, __CONTENT_OFFSET__);

        // head-> size
        this-> write <uint32_t> (__CONTENT_OFFSET__, size - __CONTENT_OFFSET__);
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

    void FreeList::erase (const std::string & dpath) {
        this-> dispose ();
        auto path = dpath + std::to_string (this-> _id);
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
            this-> write <uint32_t> (offset, reqSize);
            return offset + sizeof (uint32_t);
        }

        return 0;
    }

    bool FreeList::free (uint32_t offset) {
        uint32_t len = this-> read <uint32_t> (offset - sizeof (uint32_t));
        return this-> performFree (offset - sizeof (uint32_t), len);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          INCREASE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void FreeList::increaseSize (uint32_t size) {
        uint32_t oldSize = this-> read <uint32_t> (__SIZE_OFFSET__);
        if (oldSize < size) {
            ftruncate (fileno (this-> _handle), size);
            this-> write <uint32_t> (__SIZE_OFFSET__, size);
            this-> performFree (oldSize, size - oldSize);
        }
    }

    void FreeList::decreaseSize (uint32_t size) {
        uint32_t oldSize = this-> read <uint32_t> (__SIZE_OFFSET__);

        if (oldSize > size) {
            uint32_t nodeOffset = this-> read <uint32_t> (__HEAD_OFFSET__);
            uint32_t prev = 0;
            while (nodeOffset != 0) {
                if (nodeOffset == size) {
                    this-> write <uint32_t> (prev + sizeof (uint32_t), 0);
                    break;
                } else if (nodeOffset <= size) {
                    node n = this-> readNodeAt (nodeOffset);
                    if (n.size + nodeOffset >= size) {
                        this-> write <uint32_t> (nodeOffset, size - nodeOffset);
                        break;
                    }

                    nodeOffset = n.next;
                } else throw std::runtime_error ("Decreasing not free nodes");
            }

            ftruncate (fileno (this-> _handle), size);
        }
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
        fseek (this-> _handle, __SIZE_OFFSET__, SEEK_SET);
        fread (&size, sizeof (uint32_t), 1, this-> _handle);
        fread (&head, sizeof (uint32_t), 1, this-> _handle);

        if (head == __CONTENT_OFFSET__) {
            fread (&head, sizeof (uint32_t), 1, this-> _handle);
            return head == size;
        }

        return false;
    }

    uint32_t FreeList::maxAllocSize () const {
        uint32_t nodeOffset = this-> read <uint32_t> (__HEAD_OFFSET__);
        uint32_t max = 0;
        while (nodeOffset != 0) {
            node n = this-> readNodeAt (nodeOffset);
            if (n.size > max) {
                max = n.size;
            }

            nodeOffset = n.next;
        }

        if (max > sizeof (uint32_t)) {
            return max - sizeof (uint32_t);
        }

        return 0;
    }

    uint32_t FreeList::remainingSize () const {
        uint32_t nodeOffset = this-> read <uint32_t> (__HEAD_OFFSET__);
        uint32_t sum = 0;
        while (nodeOffset != 0) {
            node n = this-> readNodeAt (nodeOffset);

            sum += (n.size - sizeof (uint32_t));
            nodeOffset = n.next;
        }

        return sum;
    }

    uint32_t FreeList::allSize () const {
        return this-> read <uint32_t> (__SIZE_OFFSET__);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          PRIVATE ALLOC          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool FreeList::findBestFit (uint32_t size, uint32_t & offset, uint32_t & prevOffset, uint32_t & nodeSize, uint32_t & nodeNext) const {
        uint32_t nodeOffset = this-> read <uint32_t> (sizeof (uint32_t));
        uint32_t prev = 0;
        uint32_t minFound = UINT32_MAX;
        bool found = false;

        while (nodeOffset != 0) {
            node n = this-> readNodeAt (nodeOffset);

            if (n.size >= size && (minFound > n.size || !found)) {
                offset = nodeOffset;
                prevOffset = prev;
                minFound = n.size;
                nodeSize = n.size;
                nodeNext = n.next;
                found = true;
            }

            prev = nodeOffset;
            nodeOffset = n.next;
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
            node n {.size = restSize, .next = nodeNext};
            this-> writeNodeAt (newNodeOffset, n);

            if (prevOffset != 0) {
                this-> write <uint32_t> (prevOffset + sizeof (uint32_t), newNodeOffset);
            } else {
                this-> write <uint32_t> (__HEAD_OFFSET__, newNodeOffset);
            }
        } else { // not enough size for a new node, use it all
            size = nodeSize;

            if (prevOffset != 0) {
                this-> write <uint32_t> (prevOffset + sizeof (uint32_t), nodeNext);
            } else {
                this-> write <uint32_t> (__HEAD_OFFSET__, nodeNext);
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
        uint32_t nodeOffset = this-> read <uint32_t> (__HEAD_OFFSET__);
        uint32_t prevOffset = 0, prevSize = 0;

        if (nodeOffset == 0) {
            this-> write <uint32_t> (__HEAD_OFFSET__, offset);
            this-> writeNodeAt (offset, {.size = size, .next = 0});

            return true;
        }

        while (nodeOffset != 0) {
            node n = this-> readNodeAt (nodeOffset);

            if (nodeOffset + n.size == offset) {
                if (n.next == nodeOffset + (n.size + size)) {
                    node af = this-> readNodeAt (n.next);
                    node result {.size = n.size + size + af.size, .next = af.next};
                    this-> writeNodeAt (nodeOffset, result);
                } else {
                    this-> write <uint32_t> (nodeOffset, n.size + size);
                }

                return true;
            }


            if (nodeOffset > offset) { // we past the offset of the block to free
                uint32_t newNodeOffset = offset;
                bool touchPrev = (prevOffset != 0 && prevOffset + prevSize == offset);
                bool touchNext = (newNodeOffset + size == nodeOffset);

                if (touchNext && touchPrev) { // fusion the three nodes
                    node af = this-> readNodeAt (n.next);
                    node result {.size = prevSize + size + af.size, .next = af.next};
                    this-> writeNodeAt (prevOffset, result);
                }

                else if (touchPrev) { // fusion prev and new node
                    this-> write <uint32_t> (prevOffset, prevSize + size);
                }

                else if (touchNext) { // fusion new node and next
                    node result {.size = size + n.size, .next = n.next};
                    this-> writeNodeAt (newNodeOffset, result);
                    this-> write <uint32_t> (prevOffset + sizeof (uint32_t), newNodeOffset);
                }
                
                else { // insert a new node
                    node result {.size = size, .next = nodeOffset};
                    this-> writeNodeAt (newNodeOffset, result);
                    this-> write <uint32_t> (prevOffset + sizeof (uint32_t), newNodeOffset);
                }

                return true;
            }

            // Last node but the offset of after it
            if (n.next == 0 && nodeOffset + n.size < offset) {
                this-> writeNodeAt (offset, {.size = size, .next = 0});
                this-> write <uint32_t> (nodeOffset + sizeof (uint32_t), offset);

                return true;
            }


            prevOffset = nodeOffset;
            prevSize = n.size;
            nodeOffset = n.next;
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


    FreeList::node FreeList::readNodeAt (uint32_t offset) const {
        fseek (this-> _handle, offset, SEEK_SET);
        node n;
        fread (&n.size, sizeof (uint32_t), 1, this-> _handle);
        fread (&n.next, sizeof (uint32_t), 1, this-> _handle);

        return n;
    }

    void FreeList::writeNodeAt (uint32_t offset, FreeList::node n) {
        fseek (this-> _handle, offset, SEEK_SET);
        fwrite (&n.size, sizeof (uint32_t), 1, this-> _handle);
        fwrite (&n.next, sizeof (uint32_t), 1, this-> _handle);
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
        uint32_t nodeOffset = lst.read <uint32_t> (sizeof (uint32_t));
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
