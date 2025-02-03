#include "meta.hh"
#include <rd_utils/utils/mem_size.hh>
#include <rd_utils/utils/print.hh>
#include "../global.hh"

using namespace rd_utils::utils;
using namespace kv_store::common;

namespace kv_store::memory {

    MetaRamCollection::MetaRamCollection (uint32_t maxNbSlabs)
        : _maxNbSlabs (maxNbSlabs - 1)
    {}

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
                slab-> insert (k, v);
                this-> insertMetaData (k.hash () % KVMAP_META_LIST_SIZE, slab-> getUniqId ());
                return true;
            }
        }

        // Failed to allocate in already loaded slabs
        if (this-> _loadedSlabs.size () < this-> _maxNbSlabs) {
            std::shared_ptr <KVMapRAMSlab> slab = std::make_shared <KVMapRAMSlab> ();
            slab-> insert (k, v);
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
            if (slab-> remove (k)) {
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
            auto val = slab-> find (k);
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
        auto slb = this-> getLessUsed ();
        if (slb == nullptr) return false;

        for (;;) {
            std::shared_ptr <common::Key> demoteK = nullptr;
            std::shared_ptr <common::Value> demoteV = nullptr;
            auto b = slb-> begin ();
            if (b != slb-> end ()) {
                auto kv = *b;
                store.demote (*kv.first, *kv.second);

                slb-> remove (*kv.first);
                this-> removeMetaData (kv.first-> hash () % common::KVMAP_META_LIST_SIZE, slb-> getUniqId ());

                if (slb-> maxAllocSize ().bytes () >= neededSize.bytes ()) {
                    slb-> insert (k, v);
                    this-> insertMetaData (k.hash () % KVMAP_META_LIST_SIZE, slb-> getUniqId ());
                    return true;
                }
            } else return false;
        }
    }

    void MetaRamCollection::evictSlab (HybridKVStore & store) {
        std::cout << "Evicting a slab " << std::endl;
        auto slb = this-> getLessUsed ();
        if (slb == nullptr) return;

        auto hash = this-> removeSlab (slb-> getUniqId ());
        store.getDiskColl ().createSlabFromRAM (*slb, hash);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          META-DATA          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaRamCollection::insertMetaData (uint64_t hash, uint32_t slabId) {
        auto usd = this-> _used.find (slabId);
        if (usd == this-> _used.end ()) {
            this-> _used.emplace (slabId, 0);
        } else {
            usd-> second += 1;
        }

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

        std::unordered_map <uint32_t, uint32_t> lst = {{slabId, 1}};
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

    std::shared_ptr <KVMapRAMSlab> MetaRamCollection::getLessUsed () {
        std::shared_ptr <KVMapRAMSlab> result = nullptr;
        uint64_t nb = 0xFFFFFFFFFFFFFFFF;
        uint32_t id = 0;
        for (auto &it : this-> _used) {
            if (it.second < nb) {
                id = it.first;
                nb = it.second;
            }
        }

        if (id != 0) {
            return this-> _loadedSlabs [id];
        }

        return nullptr;
    }

    std::set <uint64_t> MetaRamCollection::removeSlab (uint32_t id) {
        this-> _loadedSlabs.erase (id);
        this-> _used.erase (id);
        std::set <uint64_t> hashs;

        for (auto it = this-> _slabHeads.cbegin(); it != this-> _slabHeads.cend() /* not hoisted */; /* no increment */) {
            if (it-> second.find (id) != it-> second.end ()) {
                hashs.emplace (it-> first);
            }

            this-> _slabHeads [it-> first].erase (id);
            if (it-> second.size () == 0) {
                this-> _slabHeads.erase(it++);    // or "it = m.erase(it)" since C++11
            } else {
                ++it;
            }
        }

        return hashs;
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

        return s;
    }

}
