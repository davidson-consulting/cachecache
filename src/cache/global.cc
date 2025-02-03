#include "global.hh"


namespace kv_store {

    HybridKVStore::HybridKVStore (uint32_t maxRamSlabs)
        : _ramColl (maxRamSlabs)
    {}

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


    disk::MetaDiskCollection & HybridKVStore::getDiskColl () {
        return this-> _diskColl;
    }


    std::ostream & operator<< (std::ostream & s, const kv_store::HybridKVStore & coll) {
        s << "RAM : {" << coll._ramColl << "}" << std::endl;
        s << "Disk : {" << coll._diskColl << "}";

        return s;
    }


}
