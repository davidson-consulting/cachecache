#define LOG_LEVEL 10

#include "service.hh"
#include <rd_utils/foreign/CLI11.hh>

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;

using namespace cachecache::utils;

namespace cachecache::client {

  std::shared_ptr <actor::ActorSystem> CacheService::__GLOBAL_SYSTEM__ = nullptr;

  /**
   * ========================================================================
   * ========================================================================
   * =======================    DEPLOYEMENT   ===============================
   * ========================================================================
   * ========================================================================
   */

  void CacheService::deploy (int argc, char ** argv) {
    LOG_INFO ("Starting deployement of a Cache actor");
    auto configFile = CacheService::appOption (argc, argv);

    auto repo = toml::parseFile (configFile);
    std::string addr = "0.0.0.0";
    uint32_t port = 0;
    if (repo-> contains ("sys")) {
      addr = (*repo) ["sys"].getOr ("addr", "0.0.0.0");
      port = (*repo) ["sys"].getOr ("port", 0);
    }

    __GLOBAL_SYSTEM__ = std::make_shared <actor::ActorSystem> (net::SockAddrV4 (addr, port), 2);
    __GLOBAL_SYSTEM__-> start ();

    LOG_INFO ("Starting service system : ", addr, ":", __GLOBAL_SYSTEM__-> port ());

    __GLOBAL_SYSTEM__-> add <CacheService> ("client", repo);

    __GLOBAL_SYSTEM__-> join ();
    __GLOBAL_SYSTEM__-> dispose ();
  }

  std::string CacheService::appOption (int argc, char ** argv) {
    CLI::App app;
    std::string filename = "./config.toml";
    app.add_option("-c,--config-path", filename, "the path of the configuration file");
    try {
      app.parse (argc, argv);
    } catch (const CLI::ParseError &e) {
      ::exit (app.exit (e));
    }

    return filename;
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * =========================   CONFIGURATION   ==============================
   * ==========================================================================
   * ==========================================================================
   */


  CacheService::CacheService (const std::string & name, actor::ActorSystem * sys, const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg) :
    actor::ActorBase (name, sys)
  {
    LOG_INFO ("Spawning a new cache instance -> ", name);
    this-> _cfg = cfg;
  }

  void CacheService::onStart () {
    this-> connectSupervisor (this-> _cfg);

    // no need to keep the configuration
    this-> _cfg = nullptr;
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * =========================     MESSAGING     ==============================
   * ==========================================================================
   * ==========================================================================
   */


  void CacheService::onMessage (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", RequestIds::NONE);
    LOG_INFO ("Cache instance (", this-> _name, ") received a message : ", type);

    switch (type) {
    case RequestIds::POISON_PILL :
      this-> exit ();
      break;
    default:
      LOG_ERROR ("Unkown message");
      break;
    }
  }

  std::shared_ptr<config::ConfigNode> CacheService::onRequest (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", RequestIds::NONE);
    LOG_INFO ("Cache instance (", this-> _name, ") received a request : ", type);

    return ResponseCode (404);
  }

  void CacheService::onQuit () {
    LOG_INFO ("Killing cache instance -> ", this-> _name);
    ::exit (0);
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * =========================     SUPERVISOR     ==============================
   * ==========================================================================
   * ==========================================================================
   */

  void CacheService::connectSupervisor (const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg) {
    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> act = nullptr;
    try {
      auto & conf = *cfg;
      std::string sName = "supervisor", sAddr = "127.0.0.1";
      int64_t sPort = 8080;

      if (conf.contains ("supervisor")) {
        sName = conf ["supervisor"].getOr ("name", sName);
        sAddr = conf ["supervisor"].getOr ("addr", sAddr);
        sPort = conf ["supervisor"].getOr ("port", sPort);
      }

      if (conf.contains ("sys")) {
        this-> _iface = conf ["sys"].getOr ("iface", "lo");
      } else this-> _iface = "lo";

      this-> _supervisor = this-> _system-> remoteActor (sName, sAddr + ":" + std::to_string (sPort), false);

      std::string addr = rd_utils::net::Ipv4Address::getIfaceIp (this-> _iface).toString ();

      auto msg = config::Dict ()
        .insert ("type", RequestIds::REGISTER)
        .insert ("name", this-> _name)
        .insert ("addr", addr)
        .insert ("port", (int64_t) this-> _system-> port ());

      auto resp = this-> _supervisor-> request (msg);
      if (resp == nullptr || resp-> getOr ("code", 0) != 200) {
        throw std::runtime_error ("Failed to register : " + this-> _name);
      }

    } catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
      throw std::runtime_error ("Failed to connect to supervisor");
    }

    LOG_INFO ("Connected to supervisor");
  }


}
