#define LOG_LEVEL 10

#include "global.hh"
#include <rd_utils/utils/log.hh>


namespace kv_store {

    HybridKVStore::HybridKVStore (uint32_t maxRamSlabs, uint32_t slabTTL)
        :  _ramColl (maxRamSlabs, slabTTL)
        , _currentTime (0)
    {
        this-> _hasRam = (this-> _ramColl.getNbSlabs () != 0);
        LOG_INFO ("KV Store configured with ", this-> _ramColl.getNbSlabs (), " slabs in RAM");
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          COLLECTION          ===================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void HybridKVStore::insert (const common::Key & k, const common::Value & v) {
        if (this-> _hasRam) {
            WITH_LOCK (this-> _ramMutex) {
                if (!this-> _ramColl.insert (k, v)) {
                    this-> _diskColl.insert (k, v);
                    WITH_LOCK (this-> _promoteMutex) {
                        this-> _promotions.emplace (k.asString (), v.asString ());
                    }
                }
            }
        } else { // cannot have ram if there is no buffer
            WITH_LOCK (this-> _diskMutex) {
                this-> _diskColl.insert (k, v);
            }
        }
    }

    std::shared_ptr <common::Value> HybridKVStore::find (const common::Key & k) {
        std::shared_ptr <common::Value> v = nullptr;
        if (this-> _hasRam) {
            WITH_LOCK (this-> _ramMutex) {
                v = this-> _ramColl.find (k);
            }
            if (v != nullptr) return v;
        }

        WITH_LOCK (this-> _diskMutex) {
            v = this-> _diskColl.find (k);
        }

        if (v != nullptr) {
            if (this-> _hasRam) {
                WITH_LOCK (this-> _promoteMutex) {
                    this-> _promotions.emplace (k.asString (), v-> asString ());
                }
            }

            return v;
        }

        return nullptr;
    }

    void HybridKVStore::remove (const common::Key & k) {
        WITH_LOCK (this-> _ramMutex) {
            this-> _ramColl.remove (k);
        }

        WITH_LOCK (this-> _diskMutex) {
            this-> _diskColl.remove (k);
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void HybridKVStore::resizeRamColl (uint32_t maxRamSlabs) {
        uint32_t nbRemove = 0;
        WITH_LOCK (this-> _ramMutex) {
            this-> _ramColl.setNbSlabs (maxRamSlabs);
            if (maxRamSlabs < this-> _ramColl.getNbLoadedSlabs ()) {
                nbRemove = this-> _ramColl.getNbLoadedSlabs () - maxRamSlabs;
            }
        }

        for (uint32_t i = 0  ; i < nbRemove ; i++) {
            WITH_LOCK (this-> _ramMutex) {
                this-> _ramColl.evictSlab (*this);
            }
        }

        LOG_INFO ("KV Store resized with ", maxRamSlabs, " slabs in RAM - by removing ", nbRemove);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===============================          PROMOTION/EVICTION          ===============================
     * ====================================================================================================
     * ====================================================================================================
     */

    void HybridKVStore::loop () {
        this-> _currentTime += 1;
        WITH_LOCK (this-> _ramMutex) {
            this-> _ramColl.markOldSlabs (this-> _currentTime); // , *this);
        }

        std::map <std::string, std::string> toPromote;
        WITH_LOCK (this-> _promoteMutex) {
            toPromote = std::move (this-> _promotions);
        }

        if (toPromote.size () != 0) {
            LOG_INFO ("Promoting : ", toPromote.size (), " keys");
            for (auto &it : toPromote) {
                common::Key k;
                k.set (it.first);

                common::Value v;
                v.set (it.second);

                WITH_LOCK (this-> _ramMutex) {
                    if (!this-> _ramColl.insert (k, v)) {
                        this-> _ramColl.evictSlab (*this);
                        this-> _ramColl.insert (k, v);
                    }
                }

                WITH_LOCK (this-> _diskMutex) {
                    this-> _diskColl.remove (k);
                }
            }
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    disk::MetaDiskCollection & HybridKVStore::getDiskColl () {
        return this-> _diskColl;
    }

    memory::MetaRamCollection & HybridKVStore::getRamColl () {
        return this-> _ramColl;
    }

    const disk::MetaDiskCollection & HybridKVStore::getDiskColl () const {
        return this-> _diskColl;
    }

    const memory::MetaRamCollection & HybridKVStore::getRamColl () const {
        return this-> _ramColl;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          STREAMS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::ostream & operator<< (std::ostream & s, const kv_store::HybridKVStore & coll) {
        s << "RAM : {" << coll._ramColl << "}" << std::endl;
        s << "Disk : {" << coll._diskColl << "}";

        return s;
    }

}
