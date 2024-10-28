#define LOG_LEVEL 10
#define PROJECT "HTTP_SERVER"

#include "service.hh"
#include <cstdint>

using namespace httpserver;
using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet {

  void custom_log_access (const std::string & url) {
    LOG_INFO ("Accessing : ", url);
  }

  std::shared_ptr <http_response> custom_not_found (const http_request&) {
    return std::make_shared<string_response> ("not found ", 404, "text/plain");
  }

  std::shared_ptr <http_response> custom_not_allowed (const http_request&) {
    return std::make_shared<string_response> ("not allowed ", 405, "text/plain");
  }

  FrontServer::FrontServer () :
    _login (this)
  {}

  void FrontServer::configure (const config::ConfigNode & cfg) {
    int32_t port = 8080;
    if (cfg.contains ("server")) {
      port = cfg ["server"].getOr ("port", port);
    }

    auto create = create_webserver (port)
      .log_access (custom_log_access)
      .not_found_resource (custom_not_found)
      .method_not_allowed_resource (custom_not_allowed);

    this-> _server = std::make_shared<webserver> (create);

    this-> _login.disallow_all ();
    this-> _login.set_allowing ("GET", true);
    this-> _server-> register_resource ("/login", &this-> _login);
  }

  void FrontServer::start () {
    if (this-> _server != nullptr) {
      this-> _server-> start (true);
    } else throw std::runtime_error ("Server not configured");
  }

  void FrontServer::dispose () {
    this-> _server = nullptr;
  }

  FrontServer::~FrontServer () {
    this-> dispose ();
  }

}
