#define LOG_LEVEL 10

#include "service.hh"
#include <rd_utils/foreign/CLI11.hh>

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;

namespace cachecache::client {

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

    std::string superAddr = "127.0.0.1";
    uint32_t superPort = 8080;

    if (repo-> contains ("supervisor")) {
      superAddr = (*repo) ["supervisor"].getOr ("addr", "0.0.0.0");
      superPort = (*repo) ["supervisor"].getOr ("port", 0);
    }

    actor::ActorSystem sys (net::SockAddrV4 (addr, port));
    sys.start ();

    config::Dict config;
    config.insert ("super-addr", superAddr);
    config.insert ("super-port", (int64_t) superPort);

    if (repo-> contains ("service")) {
      config.insert ("service", repo-> get ("service"));
    } else {
      config.insert ("service", std::make_shared <config::Dict> ());
    }

    sys.add <CacheService> ("client", config);

    sys.join ();
    sys.dispose ();
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


  CacheService::CacheService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode & cfg) :
    actor::ActorBase (name, sys)
  {
    LOG_INFO ("Spawning a new cache instance -> ", name);
    std::cout << cfg << std::endl;
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * =========================     MESSAGING     ==============================
   * ==========================================================================
   * ==========================================================================
   */


  void CacheService::onMessage (const config::ConfigNode & msg) {
    LOG_INFO ("Cache instance (", this-> _name, ") received a message : ");
    std::cout << msg << std::endl;
  }

  std::shared_ptr<config::ConfigNode> CacheService::onRequest (const config::ConfigNode & msg) {
    LOG_INFO ("Cache instance (", this-> _name, ") received a request : ");
    std::cout << msg << std::endl;
    return nullptr;
  }

  void CacheService::onQuit () {
    LOG_INFO ("Killing cache instance -> ", this-> _name);
  }

}
