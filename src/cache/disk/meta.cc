#include "meta.hh"
#include <rd_utils/utils/mem_size.hh>
#include <rd_utils/utils/print.hh>
#include <rd_utils/utils/files.hh>

using namespace rd_utils::utils;
using namespace kv_store::common;

namespace kv_store::disk {

    MetaDiskCollection::MetaDiskCollection () {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          INSERTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MetaDiskCollection::insert (const Key & k, const Value & v) {
        for (auto it : directory_iterator (common::KVMAP_SLAB_DISK_PATH)) {
            uint32_t id = std::stoi (get_filename (it));
            KVMapDiskSlab slab (id);
            if (slab.insert (k, v)) {
                this-> _metaColl.insert (k.hash () % KVMAP_META_LIST_SIZE, id);
                return;
            }
        }

        KVMapDiskSlab newSlab;
        if (newSlab.insert (k, v)) {
            this-> _metaColl.insert (k.hash () % KVMAP_META_LIST_SIZE, newSlab.getUniqId ());
        } else {
            throw std::runtime_error ("Failed to insert in slab no memory left");
        }
    }

    void MetaDiskCollection::createSlabFromRAM (const memory::KVMapRAMSlab & ram, const std::set <uint64_t> & hashs) {
        KVMapDiskSlab newSlab (ram);
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
            KVMapDiskSlab slab ((*lst).first);
            if (slab.remove (k)) {
                this-> _metaColl.remove (h, slab.getUniqId ());
                return;
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
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */


    std::ostream & operator<< (std::ostream & s, const MetaDiskCollection & coll) {
        for (auto it : directory_iterator (common::KVMAP_SLAB_DISK_PATH)) {
            uint32_t id = std::stoi (get_filename (it));
            KVMapDiskSlab slab (id);
            s << slab << std::endl;
        }

        return s;
    }

}
