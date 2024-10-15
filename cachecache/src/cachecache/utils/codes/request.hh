#pragma once

namespace cachecache::utils {

  enum RequestIds : int64_t {
    REGISTER = 1,
    POISON_PILL,
    NONE
  };

}
