#include "slab.hh"
#include <string.h>

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace kv_store::common;

namespace kv_store::memory {

    uint32_t KVMapRAMSlab::__ID__ = 0;

    KVMapRAMSlab::KVMapRAMSlab (uint8_t * memory, uint32_t id)
        : _uniqId (id)
        , _context (memory, kv_store::common::KVMAP_SLAB_SIZE.bytes ())
    {
        this-> load ();
    }

    KVMapRAMSlab::KVMapRAMSlab ()
        : _uniqId (__ID__ + 1)
        , _context (nullptr, 0)
    {
        __ID__ += 1;
        uint8_t * memory = reinterpret_cast <uint8_t*> (malloc (kv_store::common::KVMAP_SLAB_SIZE.bytes ()));
        if (memory == nullptr) {
            throw std::runtime_error ("Out of memory");
        }

        this-> _context = FreeList (memory, kv_store::common::KVMAP_SLAB_SIZE.bytes ());
        this-> init ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          INSERT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */


    bool KVMapRAMSlab::insert (const Key & key, const Value & value) {
        uint64_t h = key.hash () % kv_store::common::NB_KVMAP_SLAB_ENTRIES;
        instance* inst = reinterpret_cast <instance*> (this-> _context.data () + sizeof (uint32_t));

        if (inst-> lst [h] == 0) {
            if (this-> createNewEntry (key, value, inst-> lst [h])) {
                inst-> len += 1;
                return true;
            }

            return false;
        }

        node* n = reinterpret_cast <node*> (this-> _context.data () + inst-> lst [h]);
        uint32_t offset = this-> insertAfter (key, value, n);
        if (offset != 0) {
            inst-> len += 1;
            return true;
        }

        return false;
    }

    bool KVMapRAMSlab::insertAfter (const Key & k, const Value & v, node* n) {
        uint8_t * keyData = reinterpret_cast <uint8_t*> (n) + sizeof (node);
        if (n-> keySize == k.len () && ::memcmp (k.data (), keyData, n-> keySize) == 0) {
            if (n-> valueSize != v.len ()) { // need to erase the k/v and reinsert
                throw std::runtime_error ("Already exists and mismatch sizes");
            }

            // values have the same size, we just update it
            uint8_t* valData = keyData + n-> keySize;
            ::memcpy (valData, v.data (), v.len ());
            return true;
        }

        if (n-> next == 0) {
            return this-> createNewEntry (k, v, n-> next);
        }

        node* next = reinterpret_cast<node*> (this-> _context.data () + n-> next);
        return this-> insertAfter (k, v, next);
    }

    bool KVMapRAMSlab::createNewEntry (const Key & k, const Value & v, uint32_t & prevPtr) {
        uint32_t offset = this-> _context.alloc (k.len () + v.len () + sizeof (node));
        if (offset == 0) {
            // No memory left in the slab
            return false;
        }

        // Update the chained list
        prevPtr = offset;

        // Write down the data of the entry
        node* n = reinterpret_cast <node*> (this-> _context.data () + offset);
        n-> next = 0;
        n-> keySize = k.len ();
        n-> valueSize = v.len ();


        uint8_t* keyData = reinterpret_cast <uint8_t*> (this-> _context.data () + offset + sizeof (node));
        uint8_t* valueData = reinterpret_cast <uint8_t*> (this-> _context.data () + offset + sizeof (node) + n-> keySize);

        ::memcpy (keyData, k.data (), k.len ());
        ::memcpy (valueData, v.data (), v.len ());
        return true;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          FIND          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void KVMapRAMSlab::getFromOffset (uint32_t offset, std::shared_ptr <common::Key> & k, std::shared_ptr <common::Value> & v) {
        const node* n = reinterpret_cast <const node*> (this-> _context.data () + offset);
        const uint8_t * keyData = reinterpret_cast <const uint8_t*> (n) + sizeof (node);
        const uint8_t * valueData = keyData + n-> keySize;

        k = std::make_shared <Key> (n-> keySize);
        ::memcpy (k-> data (), keyData, n-> keySize);

        v = std::make_shared <Value> (n-> valueSize);
        ::memcpy (v-> data (), valueData, n-> valueSize);
    }

    std::shared_ptr <Value> KVMapRAMSlab::find (const Key & key) const {
        uint64_t h = key.hash () % kv_store::common::NB_KVMAP_SLAB_ENTRIES;
        const instance* inst = reinterpret_cast <const instance*> (this-> _context.data () + sizeof (uint32_t));

        if (inst-> lst [h] != 0) { // there is a chained list for this hashmap
            const node* n = reinterpret_cast <const node*> (this-> _context.data () + inst-> lst [h]);
            return this-> findInList (key, n);
        }

        return nullptr;
    }

    std::shared_ptr <Value> KVMapRAMSlab::findInList (const Key & k, const node* n) const {
        const uint8_t * keyData = reinterpret_cast <const uint8_t*> (n) + sizeof (node);
        if (n-> keySize == k.len () && ::memcmp (k.data (), keyData, k.len ()) == 0) {
            std::shared_ptr <Value> v = std::make_shared <Value> (n-> valueSize);

            const uint8_t* valData = keyData + n-> keySize;
            ::memcpy (v-> data (), valData, v-> len ());

            return v;
        }

        if (n-> next == 0) {
            return nullptr;
        }

        const node* next = reinterpret_cast <const node*> (this-> _context.data () + n-> next);
        return this-> findInList (k, next);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          REMOVE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool KVMapRAMSlab::remove (const Key & key) {
        uint64_t h = key.hash () % kv_store::common::NB_KVMAP_SLAB_ENTRIES;
        instance* inst = reinterpret_cast <instance*> (this-> _context.data () + sizeof (uint32_t));
        if (inst-> lst [h] != 0) {
            node* n = reinterpret_cast <node*> (this-> _context.data () + inst-> lst [h]);
            if (this-> removeInList (key, n, inst-> lst [h])) {
                inst-> len -= 1;
                return true; // something was removed from the slab
            }
        }

        return false;
    }

    bool KVMapRAMSlab::removeInList (const Key & k, node *n, uint32_t & prevPtr) {
        uint8_t * keyData = reinterpret_cast <uint8_t*> (n) + sizeof (node);
        if (n-> keySize == k.len () && ::memcmp (k.data (), keyData, k.len ()) == 0) {
            uint32_t nodeOffset = prevPtr;
            prevPtr = n-> next;

            this-> _context.free (nodeOffset);
            return true;
        }

        if (n-> next != 0) {
            node* next = reinterpret_cast <node*> (this-> _context.data () + n-> next);
            return this-> removeInList (k, next, n-> next);
        }

        return false;
    }


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    const FreeList & KVMapRAMSlab::getMemory () const {
        return this-> _context;
    }


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          DISPOSING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void KVMapRAMSlab::dispose () {
        if (this-> _context.data () != nullptr) {
            ::free (this-> _context.metadata ());
            this-> _context = FreeList (nullptr, 0);
        }
    }

    KVMapRAMSlab::~KVMapRAMSlab () {
        this-> dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void KVMapRAMSlab::init () {
        if (this-> _context.data () == nullptr) {
            throw std::runtime_error ("Out of memory");
        }

        this-> _context.init ();
        uint32_t offset = this-> _context.alloc (sizeof (instance));
        uint8_t * memory = this-> _context.data () + offset;

        ::memset (memory, 0, sizeof (instance));
    }

    void KVMapRAMSlab::load () {
        if (this-> _context.data () == nullptr) {
            throw std::runtime_error ("Malformed slab");
        }

        this-> _context.load ();
    }


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::ostream & operator<< (std::ostream & s, const KVMapRAMSlab & mp) {
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

    uint32_t KVMapRAMSlab::len () const {
        const instance* inst = reinterpret_cast <const instance*> (this-> _context.data () + sizeof (uint32_t));
        return inst-> len;
    }

    MemorySize KVMapRAMSlab::maxAllocSize () const {
        return MemorySize::B (this-> _context.maxAllocSize ());
    }

    MemorySize KVMapRAMSlab::remainingSize () const {
        return MemorySize::B (this-> _context.remainingSize ());
    }

    uint32_t KVMapRAMSlab::getUniqId () const {
        return this-> _uniqId;
    }

    KVMapRAMSlab::Iterator KVMapRAMSlab::begin () const {
        const instance* inst = reinterpret_cast <const instance*> (this-> _context.data () + sizeof (uint32_t));
        for (uint32_t i = 0 ; i < kv_store::common::NB_KVMAP_SLAB_ENTRIES ; i++) {
            if (inst-> lst [i] != 0) {
                return Iterator (this, i, inst-> lst [i]);
            }
        }

        return Iterator ();
    }

    KVMapRAMSlab::Iterator KVMapRAMSlab::end () const {
        return Iterator ();
    }

    KVMapRAMSlab::Iterator::Iterator ()
        : context (nullptr)
        , currListIndex (0)
        , currListOffset (0)
    {}

    KVMapRAMSlab::Iterator::Iterator (const KVMapRAMSlab* context, uint32_t baseIndex, uint32_t listOffset)
        : context (context)
        , currListIndex (baseIndex)
        , currListOffset (listOffset)
    {}

    std::pair <std::shared_ptr <Key>, std::shared_ptr <Value> > KVMapRAMSlab::Iterator::operator* () const {
        if (this-> currListOffset != 0) {
            const node* n = reinterpret_cast <const node*> (this-> context-> _context.data () + this-> currListOffset);
            const uint8_t* keyData = reinterpret_cast <const uint8_t*> (this-> context-> _context.data () + this-> currListOffset + sizeof (node));
            const uint8_t* valData = keyData + n-> keySize;

            std::shared_ptr <Key> k = std::make_shared <Key> (n-> keySize);
            std::shared_ptr <Value> v = std::make_shared <Value> (n-> valueSize);

            ::memcpy (k-> data (), keyData, k-> len ());
            ::memcpy (v-> data (), valData, v-> len ());

            return std::make_pair <std::shared_ptr <Key>, std::shared_ptr <Value> > (std::move (k), std::move (v));
        }

        return std::make_pair <std::shared_ptr <Key>, std::shared_ptr <Value> > (nullptr, nullptr);
    }

    KVMapRAMSlab::Iterator& KVMapRAMSlab::Iterator::operator++ () {
        if (this-> currListOffset != 0) {
            const node* n = reinterpret_cast <const node*> (this-> context-> _context.data () + this-> currListOffset);
            if (n-> next == 0) {
                const instance* inst = reinterpret_cast <const instance*> (this-> context-> _context.data () + sizeof (uint32_t));
                for (uint32_t i = this-> currListIndex + 1 ; i < kv_store::common::NB_KVMAP_SLAB_ENTRIES ; i++) {
                    if (inst-> lst [i] != 0) {
                        this-> currListOffset = inst-> lst [i];
                        this-> currListIndex = i;
                        return *this;
                    }
                }

                this-> currListOffset = 0;
                this-> currListIndex = 0;
                return *this;
            } else {
                this-> currListOffset = n-> next;
                return *this;
            }
        }

        return *this;
    }

    bool operator== (const KVMapRAMSlab::Iterator & a, const KVMapRAMSlab::Iterator & b) {
        return  (a.currListIndex == b.currListIndex && a.currListOffset == b.currListOffset);
    }

    bool operator!= (const KVMapRAMSlab::Iterator & a, const KVMapRAMSlab::Iterator & b) {
        return (a.currListIndex != b.currListIndex || a.currListOffset != b.currListOffset);
    }

}
