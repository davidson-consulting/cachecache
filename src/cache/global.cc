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
            if (!this-> _ramColl.insert (k, v)) {
                this-> _ramColl.evictSlab (*this);
                this-> _ramColl.insert (k, v);
            }
        } else { // cannot have ram if there is no buffer
            this-> _diskColl.insert (k, v);
        }
    }

    std::shared_ptr <common::Value> HybridKVStore::find (const common::Key & k) {
        std::shared_ptr <common::Value> v = nullptr;
        if (this-> _hasRam) {
            v = this-> _ramColl.find (k);
            if (v != nullptr) return v;
        }

        v = this-> _diskColl.find (k);
        if (v != nullptr) {
            if (this-> _hasRam) {
                WITH_LOCK (this-> _m) {
                    this-> _promotions.emplace (k, *v);
                }
            }

            return v;
        }

        return nullptr;
    }

    void HybridKVStore::remove (const common::Key & k) {
        this-> _ramColl.remove (k);
        this-> _diskColl.remove (k);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void HybridKVStore::resizeRamColl (uint32_t maxRamSlabs) {
        this-> _ramColl.setNbSlabs (maxRamSlabs, *this);
        LOG_INFO ("KV Store resized with ", this-> _ramColl.getNbSlabs (), " slabs in RAM");
    }

    void HybridKVStore::loop () {
        this-> _currentTime += 1;
        this-> _ramColl.markOldSlabs (this-> _currentTime); // , *this);

        std::map <common::Key, common::Value> toPromote;
        WITH_LOCK (this-> _m) {
            toPromote = std::move (this-> _promotions);
        }

        if (toPromote.size () != 0) {
            for (auto &it : toPromote) {
                this-> promoteDisk (it.first, it.second);
            }
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          PROMOTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void HybridKVStore::promoteDisk (const common::Key & k, const common::Value & v) {
        if (!this-> _ramColl.insert (k, v)) {
            if (this-> _ramColl.evictUntilFit (k, v, *this)) {
                this-> _diskColl.remove (k);
            } else {} // keep on disk
        } else {
            this-> _diskColl.remove (k);
        }
    }

    void HybridKVStore::demote (const common::Key & k, const common::Value & v) {
        this-> _diskColl.insert (k, v);
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
