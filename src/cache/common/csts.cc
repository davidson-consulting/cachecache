#include "csts.hh"


namespace kv_store::common {
    const rd_utils::utils::MemorySize KVMAP_SLAB_SIZE = rd_utils::utils::MemorySize::MB (4);
    const rd_utils::utils::MemorySize MAX_VALUE_SIZE = rd_utils::utils::MemorySize::KB (1);
    const rd_utils::utils::MemorySize MAX_KEY_SIZE = rd_utils::utils::MemorySize::B (512);
    const char * KVMAP_DISK_PATH = "./.slabs/";
}
