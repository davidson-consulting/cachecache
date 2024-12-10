#include "free_list.hh"
#include <iostream>
#include <cstring>

namespace kv_store::memory {

    FreeList::FreeList (uint8_t * memory, uint32_t totalSize)
        : _memory (memory)
        , _size (totalSize)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void FreeList::init () {
        ::memset (this-> _memory, 0, this-> _size);
        instance * list = reinterpret_cast <instance*> (this-> _memory);

        list-> size = this-> _size;
        list-> head = sizeof (instance);

        node* head = reinterpret_cast <node*> (this-> _memory + list-> head);
        head-> size = this-> _size - sizeof (instance);
        head-> next = 0;
    }

    void FreeList::load () {
        if (this-> _memory == nullptr) {
            throw std::runtime_error ("Malformed free list");
        }

        instance * list = reinterpret_cast <instance*> (this-> _memory);
        this-> _size = list-> size;
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
            uint32_t* n = reinterpret_cast<uint32_t*> (this-> _memory + offset);
            n [0] = reqSize;

            return offset + sizeof (uint32_t) - sizeof (instance);
        }

        return 0;
    }

    bool FreeList::free (uint32_t offset) {
        offset = offset + sizeof (instance);
        auto size = *reinterpret_cast<uint32_t*> (this-> _memory + (offset - sizeof (uint32_t)));
        auto ret = this-> performFree (offset - sizeof (uint32_t), size);

        return ret;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool FreeList::isEmpty () {
        instance * inst = reinterpret_cast <instance*> (this-> _memory);
        if (inst-> head == sizeof (instance)) {
            auto head = reinterpret_cast <node*> (this-> _memory + inst-> head);
            return head-> size == inst-> size;
        }

        return false;
    }

    uint32_t FreeList::maxAllocSize () const {
        instance * inst = reinterpret_cast <instance*> (this-> _memory);
        uint32_t nodeOffset = inst-> head, max = 0;
        while (nodeOffset != 0) {
            const node* n = reinterpret_cast <const node*> (this-> _memory + nodeOffset);
            if (n-> size - sizeof (uint32_t) > max) {
                max = n-> size - sizeof (uint32_t);
            }

            nodeOffset = n-> next;
        }

        return max;
    }

    uint32_t FreeList::remainingSize () const {
        instance * inst = reinterpret_cast <instance*> (this-> _memory);
        uint32_t nodeOffset = inst-> head, sum = 0;
        while (nodeOffset != 0) {
            const node* n = reinterpret_cast <const node*> (this-> _memory + nodeOffset);
            sum += n-> size;
            nodeOffset = n-> next;
        }

        return sum;
    }

    uint32_t FreeList::realAllocSize (uint32_t size) {
        uint32_t reqSize = size + sizeof (uint32_t);
        if (reqSize < sizeof (node)) {
            return sizeof (node);
        }

        return reqSize;
    }

    uint8_t* FreeList::data () {
        if (this-> _memory == nullptr) return nullptr;

        return this-> _memory + sizeof (instance);
    }

    const uint8_t* FreeList::data () const {
        if (this-> _memory == nullptr) return nullptr;

        return this-> _memory + sizeof (instance);
    }

    uint8_t* FreeList::metadata () {
        return this-> _memory;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          PRIVATE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool FreeList::findBestFit (uint32_t size, uint32_t & offset, uint32_t & prevOffset) const {
        instance* inst = reinterpret_cast <instance*> (this-> _memory);
        uint32_t nodeOffset = inst-> head;

        uint32_t prev = 0;
        auto min_found = inst-> size + 1;
        bool found = false;

        while (nodeOffset != 0) {
            const node* n = reinterpret_cast <const node*> (this-> _memory + nodeOffset);
            if (n-> size >= size && (min_found > n-> size || !found)) {
                offset = nodeOffset;
                prevOffset = prev;
                min_found = n-> size;
                found = true;
            }

            prev = nodeOffset;
            nodeOffset = n-> next;
        }

        return found;
    }

    bool FreeList::performAlloc (uint32_t & size, uint32_t & offset) {
        instance * inst = reinterpret_cast <instance*> (this-> _memory);
        uint32_t prevOffset = 0, nodeOffset = 0;
        if (!this-> findBestFit (size, nodeOffset, prevOffset)) {
            return false;
        }

        node* n = reinterpret_cast <node*> (this-> _memory + nodeOffset);
        node* prev = nullptr;
        if (prevOffset != 0) {
            prev = reinterpret_cast <node*> (this-> _memory + prevOffset);
        }

        offset = nodeOffset;
        auto restSize = (n-> size - size);

        if (restSize > sizeof (node)) { // create a new node at the given offset
            uint32_t newNodeOffset = nodeOffset + size;
            node* new_node = reinterpret_cast <node*> (this-> _memory + newNodeOffset);

            new_node-> size = restSize;
            new_node-> next = n-> next;

            if (prev) { // insert in the middle of the list
                prev-> next = newNodeOffset;
            } else { // insert at the beginning
                inst-> head = newNodeOffset;
            }

            return true;
        } else { // not enough size to create a node, use it all
            size = n-> size;
            if (prev) {
                prev-> next = n-> next; // remove the node from the list
            } else { // node was the head
                inst-> head = n-> next;
            }

            return true;
        }
    }

    bool FreeList::performFree (uint32_t offset, uint32_t size) {
        instance * inst = reinterpret_cast <instance*> (this-> _memory);
        uint32_t nodeOffset = inst-> head, prevOffset = 0;
        node* prev = nullptr;

        if (nodeOffset == 0) {
            inst-> head = offset;
            auto head = reinterpret_cast <node*> (this-> _memory + offset);

            head-> size = size;
            head-> next = 0;
            return true;
        }

        while (nodeOffset != 0) {
            node* n = reinterpret_cast <node*> (this-> _memory + nodeOffset);
            if (nodeOffset + n-> size == offset) { // freeing the block just after the node
                n-> size += size;

                // the current free block is touching the next element
                if (n-> next == (nodeOffset + n-> size)) {
                    auto next = reinterpret_cast <node*> (this-> _memory + n-> next);
                    n-> size += next-> size; // fusionning them
                    n-> next = next-> next;
                }

                return true;
            }

            if (nodeOffset > offset) { // we past the offset of the block to free
                auto newNodeOffset = offset;
                auto new_node = reinterpret_cast <node*> (this-> _memory + offset);
                new_node-> size = size;

                if (prev) { // between two nodes
                    new_node-> next = nodeOffset;
                    prev-> next = newNodeOffset;
                } else { // between the beginning and the head
                    new_node-> next = nodeOffset;
                    inst-> head = newNodeOffset;
                }

                // Touching new node and next
                if (new_node-> next != 0 && newNodeOffset + new_node-> size == new_node-> next) {
                    auto next = reinterpret_cast<node*> (this-> _memory + new_node-> next);
                    new_node-> size += next-> size;
                    new_node-> next = next-> next;
                }

                if (prev && prevOffset + prev-> size == newNodeOffset) {
                    auto next = reinterpret_cast<node*> (this-> _memory + new_node-> next);
                    prev-> size += new_node-> size;
                    prev-> next = next-> next;
                }

                return true;
            }

            // last node, but the offset is past it
            if (n-> next == 0 && nodeOffset + n-> size < offset) {
                auto newNodeOffset = offset;
                auto new_node = reinterpret_cast <node*> (this-> _memory + offset);

                new_node-> size = size;
                new_node-> next = 0;
                n-> next = newNodeOffset;

                return true;
            }

            prev = n;
            prevOffset = nodeOffset;
            nodeOffset = n-> next;
        }

        return false;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::ostream & operator<<(std::ostream & s, const FreeList & lst) {
        uint8_t * memory = lst._memory;
        FreeList::instance* inst = reinterpret_cast <FreeList::instance*> (memory);
        s << "LIST{";
        int i = 0;
        auto nodeOffset = inst-> head;
        while (nodeOffset != 0) {
            const FreeList::node* n = reinterpret_cast <const FreeList::node *> (memory + nodeOffset);
            if (i != 0) { s << ", "; i += 1; }
            s << "(" << nodeOffset << "," << nodeOffset + n-> size << ")";
            nodeOffset = n-> next;
        }
        s << "}";
        return s;
    }
}
