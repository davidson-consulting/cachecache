#pragma once


#include <rd_utils/utils/config/_.hh>
#include <memory>
#include <string>
#include <cstdint>

namespace socialNet::utils {

  enum ResponseCode : int64_t {
    OK = 200,
    FORBIDDEN = 403,
    MALFORMED = 400,
    NOT_FOUND = 404,
    SERVER_ERROR = 500
  };

  std::shared_ptr <rd_utils::utils::config::ConfigNode> response (ResponseCode code, int i);

  std::shared_ptr <rd_utils::utils::config::ConfigNode> response (ResponseCode code, const std::string & msg = "");

  std::shared_ptr <rd_utils::utils::config::ConfigNode> response (ResponseCode code, std::shared_ptr <rd_utils::utils::config::ConfigNode> content);

  std::shared_ptr <rd_utils::utils::config::ConfigNode> response (ResponseCode code, rd_utils::utils::config::Dict& content);

}
