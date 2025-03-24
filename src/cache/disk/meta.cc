#ifndef LOG_LEVEL
#define LOG_LEVEL 10
#endif

#define PROJECT META_DISK

#include "meta.hh"
#include <rd_utils/utils/mem_size.hh>
#include <rd_utils/utils/print.hh>
#include <rd_utils/utils/files.hh>
#include <rd_utils/utils/log.hh>

using namespace rd_utils::utils;
using namespace kv_store::common;

namespace kv_store::disk {

    MetaDiskCollection::MetaDiskCollection (uint32_t maxNbSlabs)
        : _metaColl ()
        , _notFullSlabs ()
        , _maxNbSlabs (maxNbSlabs)
        , _nbSlabs (0)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          INSERTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaDiskCollection::insert (const Key & k, const Value & v) {
        for (auto it = this-> _notFullSlabs.cbegin () ; it != this-> _notFullSlabs.cend () ; ) {
            KVMapDiskSlab slab (*it);
            if (slab.insert (k, v)) {
                this-> _metaColl.insert (k.hash () % KVMAP_META_LIST_SIZE, *it);
                return;
            } else {
                this-> _notFullSlabs.erase (it ++);
            }
        }

        this-> _nbSlabs += 1;
        if (this-> _nbSlabs > this-> _maxNbSlabs) {
            this-> findAndEraseSlab ();
        }

        KVMapDiskSlab newSlab;
        if (newSlab.insert (k, v)) {
            this-> _notFullSlabs.emplace (newSlab.getUniqId ());
            this-> _metaColl.insert (k.hash () % KVMAP_META_LIST_SIZE, newSlab.getUniqId ());
        } else {
            throw std::runtime_error ("Failed to insert in slab no memory left");
        }
    }

    void MetaDiskCollection::createSlabFromRAM (const memory::KVMapRAMSlab & ram, const std::set <uint64_t> & hashs) {
        this-> _nbSlabs += 1;
        if (this-> _nbSlabs > this-> _maxNbSlabs) {
            this-> findAndEraseSlab ();
        }

        KVMapDiskSlab newSlab (ram);
        this-> _notFullSlabs.emplace (newSlab.getUniqId ());
        for (auto & h : hashs) {
            this-> _metaColl.insert (h, newSlab.getUniqId ());
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          REMOVE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaDiskCollection::remove (const Key & k) {
        uint32_t h = k.hash () % common::KVMAP_META_LIST_SIZE;
        auto lst = this-> _metaColl.find (h);
        while (!lst.isEnd ()) {
            try {
                KVMapDiskSlab slab ((*lst).first);
                uint32_t newLen = 0;
                if (slab.remove (k, newLen)) {
                    this-> _notFullSlabs.emplace (slab.getUniqId ());
                    this-> _metaColl.remove (h, slab.getUniqId ());

                    if (newLen == 0) {
                        LOG_INFO ("Disk slab ", slab.getUniqId (), " empty, erased");
                        slab.erase ();
                    }
                    return;
                }
            } catch (const std::runtime_error & err) {
                // Slab not found in disk, assuming it was removed to make room
                this-> _metaColl.remove (h, (*lst).first);
            }

            ++ lst;
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          FIND          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::shared_ptr <Value> MetaDiskCollection::find (const Key & k) {
        uint32_t h = k.hash () % common::KVMAP_META_LIST_SIZE;
        auto lst = this-> _metaColl.find (h);
        while (!lst.isEnd ()) {
            KVMapDiskSlab slab ((*lst).first);
            auto v = slab.find (k);
            if (v != nullptr) {
                return v;
            }

            ++ lst;
        }

        return nullptr;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          DISPOSING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaDiskCollection::dispose () {
        for (auto it : directory_iterator (common::getSlabDirPath ())) {
            rd_utils::utils::remove (it);
        }

        rd_utils::utils::remove (common::getSlabDirPath ());
    }

    MetaDiskCollection::~MetaDiskCollection () {
        this-> dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t MetaDiskCollection::getMaxSlabs () const {
        return this-> _maxNbSlabs;
    }

    std::ostream & operator<< (std::ostream & s, const MetaDiskCollection & coll) {
        for (auto it : directory_iterator (common::getSlabDirPath ())) {
            std::string filename = get_filename (it);
            if (filename.rfind("meta", 0) == std::string::npos) {
                uint32_t id = std::stoi (filename);
                KVMapDiskSlab slab (id);
                s << slab << std::endl;
            }
        }

        return s;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          PRIVATE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaDiskCollection::findAndEraseSlab () {
        int32_t minId = -1;
        MemorySize maxSize = common::KVMAP_SLAB_SIZE;
        for (auto it : directory_iterator (common::getSlabDirPath ())) {
            std::string filename = get_filename (it);
            if (filename.rfind("meta", 0) == std::string::npos) {
                uint32_t id = std::stoi (filename);
                KVMapDiskSlab slab (id);

                auto remain = slab.remainingSize ();
                if (remain < maxSize) {
                    maxSize = remain;
                    minId = id;
                }
            }
        }

        if (minId != -1) {
            LOG_INFO ("Disk usage is full, erasing one slab : ", minId);
            this-> _nbSlabs -= 1;
            KVMapDiskSlab slab (minId);
            slab.erase ();
        }
    }

}
