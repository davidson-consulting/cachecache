#include "global.hh"


namespace kv_store {

    HybridKVStore::HybridKVStore (uint32_t maxRamSlabs)
        : _ramColl (maxRamSlabs)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          COLLECTION          ===================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void HybridKVStore::insert (const common::Key & k, const common::Value & v) {
        if (!this-> _ramColl.insert (k, v)) {
            this-> _ramColl.evictSlab (*this);
            this-> _ramColl.insert (k, v);
        }
    }

    std::shared_ptr <common::Value> HybridKVStore::find (const common::Key & k) {
        auto v = this-> _ramColl.find (k);
        if (v != nullptr) return v;

        v = this-> _diskColl.find (k);
        if (v != nullptr) {
            this-> promote (k, *v);
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
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          PROMOTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void HybridKVStore::promote (const common::Key & k, const common::Value & v) {
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
