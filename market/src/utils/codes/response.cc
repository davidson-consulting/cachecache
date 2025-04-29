#include "response.hh"

namespace kv_store::utils {

  std::shared_ptr <rd_utils::utils::config::ConfigNode> ResponseCode (uint32_t code, const std::string & msg) {
    auto ret = std::make_shared <rd_utils::utils::config::Dict> ();
    ret-> insert ("code", std::make_shared <rd_utils::utils::config::Int> (code));
    if (msg != "") {
      ret-> insert ("msg", std::make_shared <rd_utils::utils::config::String> (msg));
    }

    return ret;
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> ResponseCode (uint32_t code, std::shared_ptr<rd_utils::utils::config::ConfigNode> content) {
    auto ret = std::make_shared <rd_utils::utils::config::Dict> ();
    ret-> insert ("code", std::make_shared <rd_utils::utils::config::Int> (code));

    if (content != nullptr) {
      ret-> insert ("content", content);
    }

    return ret;
  }

}
