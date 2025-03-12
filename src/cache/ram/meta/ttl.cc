#define LOG_LEVEL 10

#include "ttl.hh"
#include <rd_utils/utils/mem_size.hh>
#include <rd_utils/utils/print.hh>
#include <rd_utils/utils/log.hh>
#include "../../global.hh"


using namespace rd_utils::utils;
using namespace kv_store::common;

namespace kv_store::memory {

    TTLMetaRamCollection::TTLMetaRamCollection (uint32_t maxNbSlabs, uint32_t slabTTL)
        : _maxNbSlabs (maxNbSlabs)
        , _slabTTL (slabTTL)
    {}


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    /**
     * @returns: the maximum number of slabs in the collection
     */
    uint32_t TTLMetaRamCollection::getNbSlabs () const {
        return this-> _maxNbSlabs;
    }

    uint32_t TTLMetaRamCollection::getNbLoadedSlabs () const {
        return this-> _loadedSlabs.size ();
    }

    void TTLMetaRamCollection::setNbSlabs (uint32_t nbSlabs) {
        this-> _maxNbSlabs = nbSlabs;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          INSERTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool TTLMetaRamCollection::insert (const Key & k, const Value & v) {
        MemorySize neededSize = MemorySize::B (k.len () + v.len () + sizeof (KVMapRAMSlab::node));
        for (auto & it : this-> _loadedSlabs) {
            auto & slab = it.second;
            if (slab-> maxAllocSize ().bytes () >= neededSize.bytes ()) {
                if (slab-> insert (k, v)) {
                    this-> insertMetaData (k.hash () % KVMAP_META_LIST_SIZE, slab-> getUniqId ());
                    return true;
                }
            }
        }

        // Failed to allocate in already loaded slabs
        if (this-> _loadedSlabs.size () < this-> _maxNbSlabs) {
            LOG_INFO ("Create a new slab in RAM : ", this-> _loadedSlabs.size () + 1, "/", this-> _maxNbSlabs);
            std::shared_ptr <KVMapRAMSlab> slab = std::make_shared <KVMapRAMSlab> ();
            slab-> insert (k, v);

            this-> _used [slab-> getUniqId ()] = {.nbHits = 1, .lastTouch = this-> _currentTime};
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

    void TTLMetaRamCollection::remove (const Key & k) {
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

    std::shared_ptr <Value> TTLMetaRamCollection::find (const Key & k) {
        auto h = k.hash () % KVMAP_META_LIST_SIZE;
        auto fnd = this-> _slabHeads.find (h);
        if (fnd == this-> _slabHeads.end ()) return nullptr;

        for (auto & scd : fnd-> second) {
            auto & slab = this-> _loadedSlabs [scd.first];
            auto val = slab-> find (k);
            if (val != nullptr) {
                auto old = this-> _used [slab-> getUniqId ()];
                this-> _used [slab-> getUniqId ()] = {.nbHits = old.nbHits + 1, .lastTouch = this-> _currentTime};
                return val;
            }
        }

        return nullptr;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    rd_utils::utils::MemorySize TTLMetaRamCollection::getMemorySize () const {
        return common::KVMAP_SLAB_SIZE * this-> _maxNbSlabs;
    }

    rd_utils::utils::MemorySize TTLMetaRamCollection::getMemoryUsage () const {
        MemorySize result = MemorySize::B (0);
        for (auto & it : this-> _loadedSlabs) {
            bool consider = false;
            auto usage = this-> _used.find (it.first);
            if (usage == this-> _used.end ()) consider = true;
            else if (!usage-> second.markedVirtualEvict) {
                consider = true;
            }

            if (consider) {
                result += common::KVMAP_SLAB_SIZE - MemorySize::B (it.second-> getMemory ().remainingSize ());
            }
        }

        return result;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          EVICTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void TTLMetaRamCollection::evictSlab (HybridKVStore & store) {
        auto slb = this-> getLessUsed ();
        if (slb == nullptr) return;

        LOG_INFO ("Evicting slab : ", slb-> getUniqId (), " ", this-> _loadedSlabs.size () - 1, "/", this-> _maxNbSlabs);

        auto hash = this-> removeSlab (slb-> getUniqId ());
        store.getDiskColl ().createSlabFromRAM (*slb, hash);
    }

    void TTLMetaRamCollection::markOldSlabs (uint64_t currentTime) {
        this-> _currentTime = currentTime;
        std::vector <uint32_t> toRemove;
        for (auto & it : this-> _used) {
            if (this-> _currentTime - it.second.lastTouch > this-> _slabTTL) {
                LOG_INFO ("Old slab : ", it.first, " ", it.second.lastTouch, " < ", this-> _currentTime, " - ", this-> _slabTTL, " marked as touched");
                it.second.markedVirtualEvict = true;
            } else {
                it.second.markedVirtualEvict = false;
            }
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          META-DATA          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void TTLMetaRamCollection::insertMetaData (uint64_t hash, uint32_t slabId) {
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

    void TTLMetaRamCollection::removeMetaData (uint64_t hash, uint32_t slabId) {
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

    std::shared_ptr <KVMapRAMSlab> TTLMetaRamCollection::getLessUsed () {
        std::shared_ptr <KVMapRAMSlab> result = nullptr;
        uint64_t nb = 0xFFFFFFFFFFFFFFFF;
        uint32_t id = 0;
        for (auto &it : this-> _used) {
            if (it.second.markedVirtualEvict) {
                id = it.first; // if slab is marked as old, then return it
                break;
            }

            if (it.second.nbHits < nb) {
                id = it.first;
                nb = it.second.nbHits;
            }
        }

        if (id != 0) {
            return this-> _loadedSlabs [id];
        }

        return nullptr;
    }

    std::set <uint64_t> TTLMetaRamCollection::removeSlab (uint32_t id) {
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


    std::ostream & operator<< (std::ostream & s, const TTLMetaRamCollection & coll) {
        for (auto & it : coll._loadedSlabs) {
            s << "SLAB : " << it.second-> getUniqId () << std::endl;
            s << *(it.second) << std::endl;
        }

        return s;
    }

}
