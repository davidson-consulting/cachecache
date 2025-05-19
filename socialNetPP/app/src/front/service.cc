#define PROJECT "HTTP_SERVER"

#include "service.hh"
#include "../registry/service.hh"
#include <cstdint>
#include <thread>

using namespace httpserver;
using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet {

  void custom_log_access (const std::string & url) {
    LOG_DEBUG ("Accessing : ", url);
  }

  std::shared_ptr <http_response> custom_not_found (const http_request&) {
    return std::make_shared<string_response> ("not found ", 404, "text/plain");
  }

  std::shared_ptr <http_response> custom_not_allowed (const http_request&) {
    return std::make_shared<string_response> ("not allowed ", 405, "text/plain");
  }

  FrontServer::FrontServer ()
    : _login (this)
    , _logon (this)
    , _submit (this)
    , _homeTimelineLen (this)
    , _homeTimeline (this)
    , _userTimelineLen (this)
    , _userTimeline (this)
    , _subLen (this)
    , _subs (this)
    , _followerLen (this)
    , _followers (this)
    , _newFollow (this)
    , _rmFollow (this)
    , _isFollow (this)
  {}

  void FrontServer::configure (const config::ConfigNode & cfg) {
    int32_t sysPort = cfg ["sys"].getOr ("port", 9010);
    std::string sysAddr = cfg ["sys"].getOr ("addr", "0.0.0.0");

    this-> _sys = std::make_shared <concurrency::actor::ActorSystem> (net::SockAddrV4 (sysAddr, sysPort));
    this-> _sys-> start ();

    this-> _registry = socialNet::connectRegistry (&(*this-> _sys), cfg);
    if (this-> _registry == nullptr) {
      throw std::runtime_error ("Failed to connect to registry");
    }

    int32_t webPort = 8080;
    int32_t nbThreads = (std::thread::hardware_concurrency () / 2);
    if (cfg.contains ("server")) {
      webPort = cfg ["server"].getOr ("port", webPort);
      nbThreads = cfg ["server"].getOr ("threads", nbThreads);
      if (nbThreads <= 0) {
        nbThreads = (std::thread::hardware_concurrency () / 2);
      }
    }

    if (cfg.contains ("cache")) {
      auto addr = cfg["cache"].getOr ("addr", "localhost");
      auto port = cfg["cache"].getOr ("port", 6650);
      LOG_INFO ("Connecting cache : ", addr, ":", port);
      this-> _cache = std::make_shared <socialNet::utils::CacheClient> (addr, port);
    }

    auto create = create_webserver (webPort)
      .log_access (custom_log_access)
      .not_found_resource (custom_not_found)
      .method_not_allowed_resource (custom_not_allowed)
      .max_threads (nbThreads);

    this-> _server = std::make_shared<webserver> (create);

    this-> _login.disallow_all ();
    this-> _login.set_allowing ("POST", true);
    this-> _server-> register_resource ("/login", &this-> _login);

    this-> _logon.disallow_all ();
    this-> _logon.set_allowing ("POST", true);
    this-> _server-> register_resource ("/new", &this-> _logon);

    this-> _submit.disallow_all ();
    this-> _submit.set_allowing ("POST", true);
    this-> _server-> register_resource ("/submit", &this-> _submit);

    this-> _homeTimelineLen.disallow_all ();
    this-> _homeTimelineLen.set_allowing ("GET", true);
    this-> _server-> register_resource ("/home-timeline-len", &this-> _homeTimelineLen);

    this-> _userTimelineLen.disallow_all ();
    this-> _userTimelineLen.set_allowing ("GET", true);
    this-> _server-> register_resource ("/user-timeline-len", &this-> _userTimelineLen);

    this-> _userTimeline.disallow_all ();
    this-> _userTimeline.set_allowing ("GET", true);
    this-> _server-> register_resource ("/user-timeline", &this-> _userTimeline);

    this-> _homeTimeline.disallow_all ();
    this-> _homeTimeline.set_allowing ("GET", true);
    this-> _server-> register_resource ("/home-timeline", &this-> _homeTimeline);

    this-> _subs.disallow_all ();
    this-> _subs.set_allowing ("GET", true);
    this-> _server-> register_resource ("/subscriptions", &this-> _subs);

    this-> _subLen.disallow_all ();
    this-> _subLen.set_allowing ("GET", true);
    this-> _server-> register_resource ("/subscriptions-len", &this-> _subLen);

    this-> _followers.disallow_all ();
    this-> _followers.set_allowing ("GET", true);
    this-> _server-> register_resource ("/followers", &this-> _followers);

    this-> _followerLen.disallow_all ();
    this-> _followerLen.set_allowing ("GET", true);
    this-> _server-> register_resource ("/followers-len", &this-> _followerLen);

    this-> _newFollow.disallow_all ();
    this-> _newFollow.set_allowing ("POST", true);
    this-> _server-> register_resource ("/follow", &this-> _newFollow);

    this-> _rmFollow.disallow_all ();
    this-> _rmFollow.set_allowing ("POST", true);
    this-> _server-> register_resource ("/unfollow", &this-> _rmFollow);

    this-> _isFollow.disallow_all ();
    this-> _isFollow.set_allowing ("GET", true);
    this-> _server-> register_resource ("/is-following", &this-> _isFollow);

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

  std::shared_ptr <socialNet::utils::CacheClient> FrontServer::getCache () {
    return this-> _cache;
  }

  void FrontServer::dispose () {
    this-> _server = nullptr;
  }

  FrontServer::~FrontServer () {
    this-> dispose ();
  }

}
