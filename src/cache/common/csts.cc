#include "csts.hh"
#include <unistd.h>

namespace kv_store::common {
    const rd_utils::utils::MemorySize KVMAP_SLAB_SIZE = rd_utils::utils::MemorySize::MB (4);
    const rd_utils::utils::MemorySize KVMAP_LRU_SLAB_SIZE = rd_utils::utils::MemorySize::MB (4);
    const rd_utils::utils::MemorySize KVMAP_META_INIT_SIZE = rd_utils::utils::MemorySize::KB (4);
    const rd_utils::utils::MemorySize MAX_VALUE_SIZE = rd_utils::utils::MemorySize::KB (128);
    const rd_utils::utils::MemorySize MAX_KEY_SIZE = rd_utils::utils::MemorySize::KB (1);
    const char * KVMAP_SLAB_DISK_PATH = "./.slabs_";

    std::string getSlabDirPath () {
        return common::KVMAP_SLAB_DISK_PATH + std::to_string (getpid ()) + "/";
    }

    std::string getMetaPath () {
        return common::KVMAP_SLAB_DISK_PATH + std::to_string (getpid ()) + "/meta";
    }

}
