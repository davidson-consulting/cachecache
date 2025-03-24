#include "slab.hh"
#include <string.h>
#include "../ram/slab.hh"
#include <rd_utils/utils/files.hh>
#include <sys/types.h>
#include <unistd.h>

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace kv_store::common;

namespace kv_store::disk {

    uint32_t KVMapDiskSlab::__ID__ = 0;
    std::string KVMapDiskSlab::__SLAB_PATH__ = "";

    KVMapDiskSlab::KVMapDiskSlab (uint32_t id)
        : _uniqId (id)
        , _context (id)
    {
        if (__SLAB_PATH__ == "") {
            __SLAB_PATH__ = common::getSlabDirPath ();
            rd_utils::utils::create_directory (__SLAB_PATH__, true);
        }

        this-> load ();
    }

    KVMapDiskSlab::KVMapDiskSlab ()
        : _uniqId (__ID__ + 1)
        , _context (__ID__ + 1)
    {
        if (__SLAB_PATH__ == "") {
            __SLAB_PATH__ = common::getSlabDirPath ();
            rd_utils::utils::create_directory (__SLAB_PATH__, true);
        }

        __ID__ += 1;
        this-> init ();
    }

    KVMapDiskSlab::KVMapDiskSlab (const memory::KVMapRAMSlab & slab)
        : _uniqId (__ID__ + 1)
        , _context (__ID__ + 1)
    {
        if (__SLAB_PATH__ == "") {
            __SLAB_PATH__ = common::getSlabDirPath ();
            rd_utils::utils::create_directory (__SLAB_PATH__, true);
        }

        __ID__ += 1;
        this-> initFromMemory (slab.getMemory ().size (), slab.getMemory ().metadata ());
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          INSERT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */


    bool KVMapDiskSlab::insert (const Key & key, const Value & value) {
        uint32_t h = key.hash () % kv_store::common::NB_KVMAP_SLAB_ENTRIES;
        uint32_t n = this-> _context.read <uint32_t> ((h * sizeof (uint32_t)) + __HEAD_OFFSET__);

        if (n == 0) {
            if (this-> createNewEntry (key, value, n)) {
                this-> _context.write <uint32_t> ((h * sizeof (uint32_t)) + __HEAD_OFFSET__, n);
                return true;
            }

            return false;
        }

        if (this-> insertAfter (key, value, n)) {
            return true;
        }

        return false;
    }

    bool KVMapDiskSlab::insertAfter (const Key & k, const Value & v, uint32_t offset) {
        node n = this-> _context.read<node> (offset);

        if (n.keySize == k.len ()) {
            if (this-> _context.compare (offset + sizeof (node), k.data (), k.len ())) {
                if (n.valueSize != v.len ()) { // need to erase the k/v and reinsert
                    throw std::runtime_error ("Already exists and mismatch sizes");
                }

                this-> _context.write (offset + sizeof (node) + k.len (), v.data (), v.len ());
                return false;
            }
        }

        if (n.next == 0) {
            if (this-> createNewEntry (k, v, n.next)) {
                this-> _context.write<uint32_t> (offset + (2 * sizeof (uint32_t)), n.next);
                return true;
            }

            return false;
        }

        return this-> insertAfter (k, v, n.next);
    }

    bool KVMapDiskSlab::createNewEntry (const Key & k, const Value & v, uint32_t & result) {
        uint32_t offset = this-> _context.alloc (k.len () + v.len () + sizeof (node));

        if (offset == 0) {
            // No memory left in the slab
            return false;
        }

        // Update the chained list
        result = offset;

        // Write down the data of the entry
        node n {.keySize = k.len (), .valueSize = v.len (), .next = 0};
        this-> _context.write <node> (offset, n);
        this-> _context.write (offset + sizeof (node), k.data (), k.len ());
        this-> _context.write (offset + sizeof (node) + k.len (), v.data (), v.len ());

        this-> _context.write<uint32_t> (__LEN_OFFSET__, this-> _context.read<uint32_t> (__LEN_OFFSET__) + 1);

        return true;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          FIND          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::shared_ptr <Value> KVMapDiskSlab::find (const Key & key) const {
        uint32_t h = key.hash () % kv_store::common::NB_KVMAP_SLAB_ENTRIES;
        uint32_t offset = this-> _context.read<uint32_t> ((h * sizeof (uint32_t)) + __HEAD_OFFSET__);

        if (offset != 0) { // there is a chained list for this hashmap
            return this-> findInList (key, offset);
        }

        return nullptr;
    }

    std::shared_ptr <Value> KVMapDiskSlab::findInList (const Key & k, uint32_t offset) const {
        node n = this-> _context.read <node> (offset);
        if (n.keySize == k.len ()) {
            if (this-> _context.compare (offset + sizeof (node), k.data (), k.len ())) {
                std::shared_ptr <Value> v = std::make_shared <Value> (n.valueSize);
                this-> _context.read (offset + sizeof (node) + n.keySize, v-> data (), n.valueSize);

                return v;
            }
        }

        if (n.next == 0) {
            return nullptr;
        }

        return this-> findInList (k, n.next);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          REMOVE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool KVMapDiskSlab::remove (const Key & key, uint32_t & newLen) {
        uint32_t h = key.hash () % kv_store::common::NB_KVMAP_SLAB_ENTRIES;
        uint32_t offset = this-> _context.read<uint32_t> ((h * sizeof (uint32_t)) + __HEAD_OFFSET__);

        if (offset != 0) {
            return this-> removeInList (key, offset, (h * sizeof (uint32_t)) + __HEAD_OFFSET__, newLen);
        }

        return false;
    }

    bool KVMapDiskSlab::removeInList (const Key & k, uint32_t offset, uint32_t prevOffset, uint32_t & newLen) {
        node n = this-> _context.read <node> (offset);

        if (n.keySize == k.len ()) {
            if (this-> _context.compare (offset + sizeof (node), k.data (), k.len ())) {
                this-> _context.free (offset);
                this-> _context.write <uint32_t> (prevOffset, n.next);
                newLen = this-> _context.read<uint32_t> (__LEN_OFFSET__) - 1;

                this-> _context.write<uint32_t> (__LEN_OFFSET__, newLen);

                return true;
            }
        }

        if (n.next != 0) {
            return this-> removeInList (k, n.next, offset + (2 * sizeof (uint32_t)), newLen);
        }

        return false;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          DISPOSING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void KVMapDiskSlab::dispose () {
        this-> _context.dispose ();
    }

    void KVMapDiskSlab::erase () {
        this-> _context.erase (common::KVMAP_SLAB_DISK_PATH);
    }

    KVMapDiskSlab::~KVMapDiskSlab () {
        this-> dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void KVMapDiskSlab::init () {
        this-> _context.init (kv_store::common::KVMAP_SLAB_SIZE.bytes (), __SLAB_PATH__);
        uint32_t offset = this-> _context.alloc ((kv_store::common::NB_KVMAP_SLAB_ENTRIES + 1) * sizeof (uint32_t));
        this-> _context.set (offset, 0, (kv_store::common::NB_KVMAP_SLAB_ENTRIES + 1) * sizeof (uint32_t));
    }

    void KVMapDiskSlab::initFromMemory (uint32_t size, const uint8_t* data) {
        this-> _context.init (kv_store::common::KVMAP_SLAB_SIZE.bytes (), __SLAB_PATH__);
        this-> _context.writeMeta (0, data, size);
    }

    void KVMapDiskSlab::load () {
        this-> _context.load (__SLAB_PATH__);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::ostream & operator<< (std::ostream & s, const KVMapDiskSlab & mp) {
        s << "KVMap[" << mp.maxAllocSize ().bytes () << "/" << mp.remainingSize ().bytes () << "] {";
        uint32_t i = 0;
        for (auto it : mp) {
            if (i != 0) {
                s << ", ";
            }

            i += 1;
            s << *it.first << " => " << *it.second;
        }

        s << "}";
        return s;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          ITERATION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    MemorySize KVMapDiskSlab::maxAllocSize () const {
        return MemorySize::B (this-> _context.maxAllocSize ());
    }

    MemorySize KVMapDiskSlab::remainingSize () const {
        return MemorySize::B (this-> _context.remainingSize ());
    }

    uint32_t KVMapDiskSlab::getUniqId () const {
        return this-> _uniqId;
    }

    KVMapDiskSlab::Iterator KVMapDiskSlab::begin () const {
        for (uint32_t i = 0 ; i < kv_store::common::NB_KVMAP_SLAB_ENTRIES ; i++) {
            uint32_t n = this-> _context.read <uint32_t> ((i * sizeof (uint32_t)) + __HEAD_OFFSET__);
            if (n != 0) {
                return Iterator (this, i, n);
            }
        }

        return Iterator ();
    }

    KVMapDiskSlab::Iterator KVMapDiskSlab::end () const {
        return Iterator ();
    }

    uint32_t KVMapDiskSlab::len () const {
        return this-> _context.read <uint32_t> (__LEN_OFFSET__);
    }

    KVMapDiskSlab::Iterator::Iterator ()
        : context (nullptr)
        , currListIndex (0)
        , currListOffset (0)
    {}

    KVMapDiskSlab::Iterator::Iterator (const KVMapDiskSlab* context, uint32_t baseIndex, uint32_t listOffset)
        : context (context)
        , currListIndex (baseIndex)
        , currListOffset (listOffset)
    {}

    std::pair <std::shared_ptr <Key>, std::shared_ptr <Value> > KVMapDiskSlab::Iterator::operator* () const {
        if (this-> currListOffset != 0) {
            node n = this-> context-> _context.read <node> (this-> currListOffset);
            std::shared_ptr <Key> k = std::make_shared <Key> (n.keySize);
            std::shared_ptr <Value> v = std::make_shared <Value> (n.valueSize);

            this-> context-> _context.read (this-> currListOffset + sizeof (node), k-> data (), k-> len ());
            this-> context-> _context.read (this-> currListOffset + sizeof (node) + k-> len (), v-> data (), v-> len ());

            return std::make_pair <std::shared_ptr <Key>, std::shared_ptr <Value> > (std::move (k), std::move (v));
        }

        return std::make_pair <std::shared_ptr <Key>, std::shared_ptr <Value> > (nullptr, nullptr);
    }

    KVMapDiskSlab::Iterator& KVMapDiskSlab::Iterator::operator++ () {
        if (this-> currListOffset != 0) {
            node n = this-> context-> _context.read<node> (this-> currListOffset);
            if (n.next == 0) {
                for (uint32_t i = this-> currListIndex + 1 ; i < kv_store::common::NB_KVMAP_SLAB_ENTRIES ; i++) {
                    uint32_t off = this-> context-> _context.read <uint32_t> ((i * sizeof (uint32_t)) + __HEAD_OFFSET__);
                    if (off != 0) {
                        this-> currListOffset = off;
                        this-> currListIndex = i;
                        return *this;
                    }
                }

                this-> currListIndex = 0;
                this-> currListOffset = 0;
                return *this;
            } else {
                this-> currListOffset = n.next;
            }
        }

        return *this;
    }

    bool operator== (KVMapDiskSlab::Iterator & a, KVMapDiskSlab::Iterator & b) {
        return  (a.currListIndex == b.currListIndex && a.currListOffset == b.currListOffset);
    }

    bool operator!= (KVMapDiskSlab::Iterator & a, KVMapDiskSlab::Iterator & b) {
        return (a.currListIndex != b.currListIndex || a.currListOffset != b.currListOffset);
    }

}
