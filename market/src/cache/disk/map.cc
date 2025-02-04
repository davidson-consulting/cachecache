#include "map.hh"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace kv_store::common;


namespace kv_store::disk {

    uint32_t DiskMap::__ID__ = 0;

    DiskMap::DiskMap (uint32_t id)
        : _uniqId (id)
        , _context (id)
    {
        this-> load ();
    }

    DiskMap::DiskMap ()
        : _uniqId (__ID__ + 1)
        , _context (__ID__ + 1)
    {
        __ID__ += 1;
        this-> init ();
    }


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DiskMap::init () {
        std::string path = std::string (kv_store::common::KVMAP_META_DISK_PATH) + std::to_string (getpid ()) + "_";
        this-> _context.init (kv_store::common::KVMAP_META_INIT_SIZE.bytes (), path);
        uint32_t offset = this-> _context.alloc (kv_store::common::NB_KVMAP_SLAB_ENTRIES * sizeof (uint32_t));
        this-> _context.set (offset, 0, kv_store::common::NB_KVMAP_SLAB_ENTRIES * sizeof (uint32_t));
    }

    void DiskMap::load () {
        this-> _context.load (kv_store::common::KVMAP_META_DISK_PATH);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          DISPOSING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DiskMap::dispose () {
        this-> _context.dispose ();
    }

    void DiskMap::erase () {
        this-> _context.erase (common::KVMAP_META_DISK_PATH);
    }

    DiskMap::~DiskMap () {
        this-> dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          INSERT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DiskMap::insert (uint64_t kh, uint32_t value) {
        uint32_t h = (uint32_t) (kh % kv_store::common::NB_KVMAP_SLAB_ENTRIES);
        uint32_t n = this-> _context.read <uint32_t> ((h * sizeof (uint32_t)) + __HEAD_OFFSET__);

        if (n == 0) {
            this-> createNewEntry (value, n);
            this-> _context.write <uint32_t> ((h * sizeof (uint32_t)) + __HEAD_OFFSET__, n);

        } else {
            this-> insertAfter (value, n);
        }
    }


    void DiskMap::insertAfter (uint32_t value, uint32_t offset) {
        node n = this-> _context.read <node> (offset);
        if (n.slabId == value) {
            this-> _context.write <uint32_t> (offset + sizeof (uint32_t), n.nbs + 1);
            return;
        }

        if (n.next == 0) {
            this-> createNewEntry (value, n.next);
            this-> _context.write <uint32_t> (offset + (2 * sizeof (uint32_t)), n.next);

            return;
        }

        this-> insertAfter (value, n.next);
    }

    void DiskMap::createNewEntry (uint32_t value, uint32_t & result) {
        uint32_t offset = this-> _context.alloc (sizeof (node));
        if (offset == 0) {
            this-> _context.increaseSize (this-> _context.allSize () * 2);
            offset = this-> _context.alloc (sizeof (node));

            if (offset == 0) throw std::runtime_error ("Failed to create new entry");
        }

        result = offset;
        node n {.slabId = value, .nbs = 1, .next = 0};
        this-> _context.write <node> (offset, n);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          FIND          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    DiskMap::SlabListIterator DiskMap::find (uint64_t kh) const {
        uint32_t h = kh % kv_store::common::NB_KVMAP_SLAB_ENTRIES;
        uint32_t offset = this-> _context.read<uint32_t> ((h * sizeof (uint32_t)) + __HEAD_OFFSET__);

        if (offset != 0) { // there is a chained list for this hashmap
            return SlabListIterator (this, offset);
        }

        return SlabListIterator (nullptr, 0);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          REMOVING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DiskMap::remove (uint64_t kh, uint32_t slabId) {
        uint32_t h = kh % kv_store::common::NB_KVMAP_SLAB_ENTRIES;
        uint32_t offset = this-> _context.read<uint32_t> ((h * sizeof (uint32_t)) + __HEAD_OFFSET__);

        if (offset != 0) {
            this-> removeInList (slabId, offset, (h * sizeof (uint32_t)) + __HEAD_OFFSET__);
        }
    }

    void DiskMap::removeInList (uint32_t slabId, uint32_t offset, uint32_t prevOffset) {
        node n = this-> _context.read <node> (offset);
        if (n.slabId == slabId) {
            n.nbs -= 1;
            if (n.nbs == 0) {
                this-> _context.free (offset);
                this-> _context.write (prevOffset, n.next);
                return;
            }

            this-> _context.write (offset + sizeof (uint32_t), n.nbs);
            return;
        }

        if (n.next != 0) {
            this-> removeInList (slabId, n.next, offset + (2 * sizeof (uint32_t)));
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          ITERATION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    MemorySize DiskMap::maxAllocSize () const {
        return MemorySize::B (this-> _context.maxAllocSize ());
    }

    MemorySize DiskMap::remainingSize () const {
        return MemorySize::B (this-> _context.remainingSize ());
    }

    uint32_t DiskMap::getUniqId () const {
        return this-> _uniqId;
    }

    DiskMap::SlabListIterator::SlabListIterator () {}

    DiskMap::SlabListIterator::SlabListIterator (const DiskMap* context, uint32_t currListOffset)
        : context (context)
        , currListOffset (currListOffset)
    {}


    std::pair <uint32_t, uint32_t> DiskMap::SlabListIterator::operator* () const {
        if (this-> currListOffset != 0) {
            node n =  this-> context-> _context.read <node> (this-> currListOffset);
            return std::make_pair (n.slabId, n.nbs);
        }

        return std::make_pair (0, 0);
    }

    DiskMap::SlabListIterator& DiskMap::SlabListIterator::operator++ () {
        if (this-> currListOffset != 0) {
            auto n = this-> context-> _context.read <DiskMap::node> (this-> currListOffset);
            if (n.next != 0) {
                this-> currListOffset = n.next;
            } else {
                this-> currListOffset = 0;
                this-> context = nullptr;
            }
        }

        return *this;
    }

    bool operator== (const DiskMap::SlabListIterator & a, const DiskMap::SlabListIterator & b) {
        return a.context == b.context && a.currListOffset == b.currListOffset;
    }

    bool operator!= (const DiskMap::SlabListIterator & a, const DiskMap::SlabListIterator & b) {
        return a.context != b.context || a.currListOffset != b.currListOffset;
    }

    bool DiskMap::SlabListIterator::isEnd () const {
        return this-> context == nullptr && this-> currListOffset == 0;
    }


}
