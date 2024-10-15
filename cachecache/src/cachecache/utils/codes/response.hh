#pragma once

#include <rd_utils/utils/config/_.hh>
#include <memory>
#include <string>

namespace cachecache::utils {

  std::shared_ptr <rd_utils::utils::config::ConfigNode> ResponseCode (uint32_t code, const std::string & msg = "");

  std::shared_ptr <rd_utils::utils::config::ConfigNode> ResponseCode (uint32_t code, std::shared_ptr <rd_utils::utils::config::ConfigNode> content);

}
