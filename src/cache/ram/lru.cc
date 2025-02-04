#include "lru.hh"
#include "../common/csts.hh"

namespace kv_store::memory {


    SlabLRU::SlabLRU ()
        : _context (nullptr, 0)
    {
        uint8_t * memory = reinterpret_cast <uint8_t*> (malloc (kv_store::common::KVMAP_LRU_SLAB_SIZE.bytes ()));
        if (memory == nullptr) {
            throw std::runtime_error ("Out of memory");
        }

        this-> _context = FreeList (memory, kv_store::common::KVMAP_LRU_SLAB_SIZE.bytes ());
        this-> _context.init ();
        uint32_t offset = this-> _context.alloc (sizeof (uint32_t) * 2);

        this-> _head = reinterpret_cast <uint32_t*> (this-> _context.data () + offset);
        this-> _head [0] = 0;
        this-> _head [1] = 0; // tail
                              //
    }

    uint32_t SlabLRU::insert (uint32_t slabId, uint32_t slabOff) {
        uint32_t offset = this-> _context.alloc (sizeof (LRUInfo));
        if (offset == 0) {
            throw std::runtime_error ("Failed to allocate in LRU no memory left");
        }

        LRUInfo * info = reinterpret_cast <LRUInfo*> (this-> _context.data () + offset);
        info-> slabId = slabId;
        info-> offset = slabOff;

        if (this-> _head [0] == 0) {
            this-> _head [0] = offset;
            this-> _head [1] = offset;
            info-> next = 0;
            info-> prev = 0;

            return offset;
        }

        LRUInfo * prevHead = reinterpret_cast <LRUInfo*> (this-> _context.data () + this-> _head [0]);
        prevHead-> prev = offset;
        info-> next = this-> _head [0];
        info-> prev = 0;
        this-> _head [0] = offset;

        return offset;
    }

    void SlabLRU::pushFront (uint32_t offset) {
        LRUInfo * info = reinterpret_cast <LRUInfo*> (this-> _context.data () + offset);
        if (info-> next == 0) return ; // already the tail
        if (info-> prev != 0) {
            LRUInfo * prev = reinterpret_cast <LRUInfo*> (this-> _context.data () + info-> prev);
            prev-> next = info-> next;
        } else {
            this-> _head [0] = info-> next;
        }

        LRUInfo * next = reinterpret_cast <LRUInfo*> (this-> _context.data () + info-> next);
        next-> prev = info-> prev;

        LRUInfo * tail = reinterpret_cast <LRUInfo*> (this-> _context.data () + this-> _head [1]);
        tail-> next = offset;

        info-> prev = this-> _head [1];
        info-> next = 0;
        this-> _head [1] = offset;
    }

    void SlabLRU::erase (uint32_t offset) {
        LRUInfo * info = reinterpret_cast <LRUInfo*> (this-> _context.data () + offset);
        if (info-> prev != 0) {
            LRUInfo * prev = reinterpret_cast <LRUInfo*> (this-> _context.data () + info-> prev);
            prev-> next = info-> next;
        } else {
            this-> _head [0] = info-> next;
        }

        if (info-> next != 0) {
            LRUInfo * next = reinterpret_cast <LRUInfo*> (this-> _context.data () + info-> next);
            next-> prev = info-> prev;
        } else {
            this-> _head [1] = info-> prev;
        }

        this-> _context.free (offset);
    }

    bool SlabLRU::getOldest (SlabLRU::LRUInfo & info) const {
        if (this-> _head [0] == 0) {
            return false;
        }

        info = *(reinterpret_cast <const LRUInfo*> (this-> _context.data () + this-> _head [0]));
        return true;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          DISPOSING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void SlabLRU::dispose () {
        if (this-> _context.data () != nullptr) {
            ::free (this-> _context.metadata ());
            this-> _context = FreeList (nullptr, 0);
        }
    }

    SlabLRU::~SlabLRU () {
        this-> dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::ostream & operator<< (std::ostream & s, const SlabLRU & lru) {
        s << "LRU [";
        auto current = lru._head [0];
        while (current != 0) {
            const SlabLRU::LRUInfo * node = reinterpret_cast <const SlabLRU::LRUInfo*> (lru._context.data () + current);
            s << "(" << node-> slabId << ";" << node-> offset << ")";
            current = node-> next;
        }

        s << "]";
        return s;
    }

}
