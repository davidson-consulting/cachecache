#include "response.hh"

namespace socialNet::utils {

std::shared_ptr <rd_utils::utils::config::ConfigNode> response (ResponseCode code, int msg) {
    auto ret = std::make_shared <rd_utils::utils::config::Dict> ();
    ret-> insert ("code", std::make_shared <rd_utils::utils::config::Int> (code));
    ret-> insert ("content", std::make_shared <rd_utils::utils::config::Int> (msg));

    return ret;
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> response (ResponseCode code, const std::string & msg) {
    auto ret = std::make_shared <rd_utils::utils::config::Dict> ();
    ret-> insert ("code", std::make_shared <rd_utils::utils::config::Int> (code));
    if (msg != "") {
      ret-> insert ("content", std::make_shared <rd_utils::utils::config::String> (msg));
    }

    return ret;
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> response (ResponseCode code, std::shared_ptr<rd_utils::utils::config::ConfigNode> content) {
    auto ret = std::make_shared <rd_utils::utils::config::Dict> ();
    ret-> insert ("code", std::make_shared <rd_utils::utils::config::Int> (code));

    if (content != nullptr) {
      ret-> insert ("content", content);
    }

    return ret;
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> response (ResponseCode code, rd_utils::utils::config::Dict & content) {
    auto ret = std::make_shared <rd_utils::utils::config::Dict> ();
    ret-> insert ("code", std::make_shared <rd_utils::utils::config::Int> (code));
    ret-> insert ("content", std::make_shared <rd_utils::utils::config::Dict> (content));

    return ret;
  }

}
