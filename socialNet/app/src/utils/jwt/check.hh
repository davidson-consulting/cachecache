#pragma once

#include <rd_utils/_.hh>

namespace socialNet::utils {

  bool checkConnected (const rd_utils::utils::config::ConfigNode & msg, const std::string & issuer, const std::string & secret);
  std::string createJWTToken (uint32_t id, const std::string & issuer, const std::string & secret);

}
