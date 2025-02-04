#pragma once

#include <cstdint>

namespace kv_store::utils {

  enum RequestIds : int64_t {
    REGISTER = 1,
    EXIT,
    ENTITY_INFO,
    UPDATE_SIZE,
    POISON_PILL,
    NONE
  };

}
