#pragma once

namespace cachecache::utils {

  enum RequestIds : int64_t {
    REGISTER = 1,
    EXIT,
    POISON_PILL,
    NONE
  };

}
