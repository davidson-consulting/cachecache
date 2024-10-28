#define LOG_LEVEL 10
#define PROJECT "HTTP_SERVER"

#include "service.hh"
#include "../registry/service.hh"
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
    , _logon (this)
  {}

  void FrontServer::configure (const config::ConfigNode & cfg) {
    int32_t sysPort = cfg ["sys"].getOr ("port", 9010);
    std::string sysAddr = cfg ["sys"].getOr ("addr", "0.0.0.0");
    this-> _sys = std::make_shared <concurrency::actor::ActorSystem> (net::SockAddrV4 (sysAddr, sysPort));
    this-> _sys-> start ();

    this-> _registry = socialNet::connectRegistry (&(*this-> _sys), cfg);

    int32_t webPort = 8080;
    if (cfg.contains ("server")) {
      webPort = cfg ["server"].getOr ("port", webPort);
    }

    auto create = create_webserver (webPort)
      .log_access (custom_log_access)
      .not_found_resource (custom_not_found)
      .method_not_allowed_resource (custom_not_allowed);

    this-> _server = std::make_shared<webserver> (create);

    this-> _login.disallow_all ();
    this-> _login.set_allowing ("POST", true);
    this-> _server-> register_resource ("/login", &this-> _login);

    this-> _logon.disallow_all ();
    this-> _logon.set_allowing ("POST", true);
    this-> _server-> register_resource ("/new", &this-> _logon);

    LOG_INFO ("Web service ready on port ", webPort);
  }

  void FrontServer::start () {
    if (this-> _server != nullptr) {
      this-> _server-> start (true);
    } else throw std::runtime_error ("Server not configured");
  }

  rd_utils::concurrency::actor::ActorSystem* FrontServer::getSystem () {
    return &(*this-> _sys);
  }

  std::shared_ptr <rd_utils::concurrency::actor::ActorRef> FrontServer::getRegistry () {
    return this-> _registry;
  }

  void FrontServer::dispose () {
    this-> _server = nullptr;
  }

  FrontServer::~FrontServer () {
    this-> dispose ();
  }

}
