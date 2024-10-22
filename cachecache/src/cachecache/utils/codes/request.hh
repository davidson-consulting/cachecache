#pragma once

namespace cachecache::utils {

  enum RequestIds : int64_t {
    REGISTER = 1,
    EXIT,
    ENTITY_INFO,
    UPDATE_SIZE,
    POISON_PILL,
    NONE
  };

}
