#define LOG_LEVEL 10

#include "global.hh"
#include <rd_utils/utils/log.hh>


namespace kv_store {

    HybridKVStore::HybridKVStore (uint32_t maxRamSlabs, uint32_t slabTTL)
        : _bufferColl (HybridKVStore::computeBufferSize (maxRamSlabs), 0) // no TTL on buffer collection, and only one slab
        , _ramColl (HybridKVStore::computeRamSize (maxRamSlabs), slabTTL)
        , _currentTime (0)
    {
        this-> _hasRam = (this-> _ramColl.getNbSlabs () != 0);
        this-> _hasBuffer = (this-> _bufferColl.getNbSlabs () != 0);
        LOG_INFO ("KV Store configured with ", this-> _bufferColl.getNbSlabs (), " slabs in buffer, and ", this-> _ramColl.getNbSlabs (), " slabs in RAM");
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          COLLECTION          ===================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void HybridKVStore::insert (const common::Key & k, const common::Value & v) {
        if (this-> _hasBuffer) {
            if (!this-> _bufferColl.insert (k, v)) {
                this-> _bufferColl.evictSlab (*this);
                this-> _bufferColl.insert (k, v);
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

        if (this-> _hasBuffer) {
            v = this-> _bufferColl.find (k);
            if (v != nullptr) {
                if (this-> _hasRam) this-> promoteBuffer (k, *v);
                return v;
            }
        }

        v = this-> _diskColl.find (k);
        if (v != nullptr) {
            if (this-> _hasRam) this-> promoteDisk (k, *v);
            return v;
        }

        return nullptr;
    }

    void HybridKVStore::remove (const common::Key & k) {
        this-> _bufferColl.remove (k);
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
        this-> _bufferColl.setNbSlabs (HybridKVStore::computeBufferSize (maxRamSlabs), *this);
        this-> _ramColl.setNbSlabs (HybridKVStore::computeRamSize (maxRamSlabs), *this);
        LOG_INFO ("KV Store resized with ", this-> _bufferColl.getNbSlabs (), " slabs in buffer, and ", this-> _ramColl.getNbSlabs (), " slabs in RAM");
    }

    void HybridKVStore::loop () {
        this-> _currentTime += 1;
        this-> _ramColl.evictOldSlabs (this-> _currentTime, *this);
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

    void HybridKVStore::promoteBuffer (const common::Key & k, const common::Value & v) {
        if (!this-> _ramColl.insert (k, v)) {
            if (this-> _ramColl.evictUntilFit (k, v, *this)) { // does not fit on ram keep on buffer
                this-> _bufferColl.remove (k);
            }
        } else {
            this-> _bufferColl.remove (k);
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
        s << "BUFFER : {" << coll._bufferColl << "}" << std::endl;
        s << "RAM : {" << coll._ramColl << "}" << std::endl;
        s << "Disk : {" << coll._diskColl << "}";

        return s;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          SIZES          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */


    uint32_t HybridKVStore::computeRamSize (uint32_t maxNbSlabs) {
        std::cout << maxNbSlabs << std::endl;
        if (maxNbSlabs <= 1) return 0;
        return maxNbSlabs - 1;
    }

    uint32_t HybridKVStore::computeBufferSize (uint32_t maxNbSlabs) {
        if (maxNbSlabs == 0) return 0;
        return 1;
    }

}
