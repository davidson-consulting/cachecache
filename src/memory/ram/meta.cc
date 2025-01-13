#include "meta.hh"
#include <rd_utils/utils/mem_size.hh>
#include <rd_utils/utils/print.hh>

using namespace rd_utils::utils;

namespace kv_store::memory {

    MetaRamCollection::MetaRamCollection (uint32_t maxNbSlabs)
        : _maxNbSlabs (maxNbSlabs)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          INSERTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaRamCollection::insert (const Key & k, const Value & v) {
        MemorySize neededSize = MemorySize::B (k.len () + v.len () + sizeof (KVMapSlab::node));
        for (auto & it : this-> _loadedSlabs) {
            auto & slab = it.second;
            if (slab-> maxAllocSize () > neededSize || slab-> maxAllocSize () == neededSize) {
                slab-> alloc (k, v);
                this-> insertMetaData (k.hash (), slab-> getUniqId ());
                return;
            }
        }

        // Failed to allocate in already loaded slabs
        if (this-> _loadedSlabs.size () < this-> _maxNbSlabs) {
            std::shared_ptr <KVMapSlab> slab = std::make_shared <KVMapSlab> ();
            slab-> alloc (k, v);
            this-> _loadedSlabs.emplace (slab-> getUniqId (), slab);
            this-> insertMetaData (k.hash (), slab-> getUniqId ());
            return;
        }

        throw std::runtime_error ("Failed to insert in slab no memory left");
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          REMOVE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaRamCollection::remove (const Key & k) {
        auto h =  k.hash ();
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
        auto h = k.hash ();
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

        return s;
    }

}
