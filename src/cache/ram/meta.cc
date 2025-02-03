#include "meta.hh"
#include <rd_utils/utils/mem_size.hh>
#include <rd_utils/utils/print.hh>
#include "../global.hh"

using namespace rd_utils::utils;
using namespace kv_store::common;

namespace kv_store::memory {

    MetaRamCollection::MetaRamCollection (uint32_t maxNbSlabs)
        : _maxNbSlabs (maxNbSlabs - 1)
    {
        if (maxNbSlabs < 2) {
            throw std::runtime_error ("At least 2 slabs are mandatory (a slab for LRU and a slab for values)");
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          INSERTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool MetaRamCollection::insert (const Key & k, const Value & v) {
        MemorySize neededSize = MemorySize::B (k.len () + v.len () + sizeof (KVMapRAMSlab::node));
        for (auto & it : this-> _loadedSlabs) {
            auto & slab = it.second;
            if (slab-> maxAllocSize ().bytes () >= neededSize.bytes ()) {
                slab-> insert (k, v, this-> _lru);
                this-> insertMetaData (k.hash () % KVMAP_META_LIST_SIZE, slab-> getUniqId ());
                return true;
            }
        }

        // Failed to allocate in already loaded slabs
        if (this-> _loadedSlabs.size () < this-> _maxNbSlabs) {
            std::shared_ptr <KVMapRAMSlab> slab = std::make_shared <KVMapRAMSlab> ();
            slab-> insert (k, v, this-> _lru);
            this-> _loadedSlabs.emplace (slab-> getUniqId (), slab);
            this-> insertMetaData (k.hash () % KVMAP_META_LIST_SIZE, slab-> getUniqId ());
            return true;
        }

        return false;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          REMOVE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaRamCollection::remove (const Key & k) {
        auto h =  k.hash () % common::KVMAP_META_LIST_SIZE;
        auto fnd = this-> _slabHeads.find (h);
        if (fnd == this-> _slabHeads.end ()) return;

        for (auto & scd : fnd-> second) {
            auto & slab = this-> _loadedSlabs [scd.first];
            if (slab-> remove (k, this-> _lru)) {
                this-> removeMetaData (h, slab-> getUniqId ());
                return;
            }
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          FIND          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::shared_ptr <Value> MetaRamCollection::find (const Key & k) {
        auto h = k.hash () % KVMAP_META_LIST_SIZE;
        auto fnd = this-> _slabHeads.find (h);
        if (fnd == this-> _slabHeads.end ()) return nullptr;

        for (auto & scd : fnd-> second) {
            auto & slab = this-> _loadedSlabs [scd.first];
            auto val = slab-> find (k, this-> _lru);
            if (val != nullptr) return val;
        }

        return nullptr;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          EVICTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool MetaRamCollection::evictUntilFit (const common::Key & k, const common::Value & v, HybridKVStore & store) {
        MemorySize neededSize = MemorySize::B (k.len () + v.len () + sizeof (KVMapRAMSlab::node));
        for (;;) {
            SlabLRU::LRUInfo info;
            if (this-> _lru.getOldest (info)) {
                std::shared_ptr <common::Key> demoteK = nullptr;
                std::shared_ptr <common::Value> demoteV = nullptr;
                auto slb = this-> _loadedSlabs.find (info.slabId);
                if (slb == this-> _loadedSlabs.end ()) throw std::runtime_error ("Malformed lru");

                slb-> second-> getFromOffset (info.offset, demoteK, demoteV);
                store.demote (*demoteK, *demoteV);

                std::cout << "Eviction : " << *demoteK << std::endl;

                slb-> second-> remove (*demoteK, this-> _lru);
                this-> removeMetaData (demoteK-> hash () % common::KVMAP_META_LIST_SIZE, slb-> second-> getUniqId ());
                if (slb-> second-> maxAllocSize ().bytes () >= neededSize.bytes ()) {
                    slb-> second-> insert (k, v, this-> _lru);
                    this-> insertMetaData (k.hash () % KVMAP_META_LIST_SIZE, slb-> second-> getUniqId ());
                    return true;
                }
            } else return false;
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          META-DATA          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaRamCollection::insertMetaData (uint64_t hash, uint32_t slabId) {
        auto fnd = this-> _slabHeads.find (hash);

        if (fnd != this-> _slabHeads.end ()) {
            auto scd = fnd-> second.find (slabId);
            if (scd == fnd-> second.end ()) {
                fnd-> second.emplace (slabId, 1);
            } else {
                fnd-> second [slabId] += 1;
            }

            return;
        }

        std::map <uint32_t, uint32_t> lst = {{slabId, 1}};
        this-> _slabHeads.emplace (hash, lst);
    }

    void MetaRamCollection::removeMetaData (uint64_t hash, uint32_t slabId) {
        auto fnd = this-> _slabHeads.find (hash);
        if (fnd != this-> _slabHeads.end ()) {
            auto scd = fnd-> second.find (slabId);
            if (scd != fnd-> second.end ()) {
                auto nb = scd-> second;
                if (nb == 1) {
                    fnd-> second.erase (slabId);
                    if (fnd-> second.size () == 0) {
                        this-> _slabHeads.erase (hash);
                    }
                } else {
                    fnd-> second [slabId] -= 1;
                }
            } 
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */


    std::ostream & operator<< (std::ostream & s, const MetaRamCollection & coll) {
        for (auto & it : coll._loadedSlabs) {
            s << "SLAB : " << it.second-> getUniqId () << std::endl;
            s << *(it.second) << std::endl;
        }
        s << coll._lru << std::endl;

        return s;
    }

}
