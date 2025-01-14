#pragma once

#include <string>
#include <rd_utils/utils/mem_size.hh>

namespace kv_store::common {
    constexpr int NB_KVMAP_SLAB_ENTRIES = 512;
    extern const rd_utils::utils::MemorySize KVMAP_SLAB_SIZE;// = rd_utils::utils::MemorySize::MB (4);
    extern const rd_utils::utils::MemorySize MAX_VALUE_SIZE;// = rd_utils::utils::MemorySize::KB (1);
    extern const rd_utils::utils::MemorySize MAX_KEY_SIZE;// = rd_utils::utils::MemorySize::B (512);
    extern const char * KVMAP_DISK_PATH;
}
