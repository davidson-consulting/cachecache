#define LOG_LEVEL 10

#include "service.hh"

#include <cachecache/client/service.hh>
#include <rd_utils/foreign/CLI11.hh>


using namespace rd_utils;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;

namespace cachecache::supervisor {

  /**
   * ========================================================================
   * ========================================================================
   * =======================    DEPLOYEMENT   ===============================
   * ========================================================================
   * ========================================================================
   */

  void SupervisorService::deploy (int argc, char ** argv) {
    LOG_INFO ("Starting deployement of a Supervisor actor");
    auto configFile = SupervisorService::appOption (argc, argv);

    auto repo = toml::parseFile (configFile);
    std::string addr = "0.0.0.0";
    uint32_t port = 0;
    if (repo-> contains ("sys")) {
      addr = (*repo) ["sys"].getOr ("addr", "0.0.0.0");
      port = (*repo) ["sys"].getOr ("port", 0);
    }

    actor::ActorSystem sys (net::SockAddrV4 (addr, port));
    sys.start ();

    config::Dict config ;
    if (repo-> contains ("supervisor")) {
      config.insert ("config", repo-> get ("supervisor"));
    }

    sys.add <SupervisorService> ("supervisor", config);



    sys.join ();
    sys.dispose ();
  }

  std::string SupervisorService::appOption (int argc, char ** argv) {
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


  SupervisorService::SupervisorService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode & cfg) :
    actor::ActorBase (name, sys)
  {
    LOG_INFO ("Spawning supervisor actor -> ", name);
    std::cout << cfg << std::endl;
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * =========================     MESSAGING     ==============================
   * ==========================================================================
   * ==========================================================================
   */

  void SupervisorService::onMessage (const rd_utils::utils::config::ConfigNode & msg) {
    LOG_INFO ("Supervisor actor (", this-> _name, ") received a message : ");
    std::cout << msg << std::endl;
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> SupervisorService::onRequest (const rd_utils::utils::config::ConfigNode & msg) {
    LOG_INFO ("Supervisor actor (", this-> _name, ") received a request : ");
    std::cout << msg << std::endl;
    return nullptr;
  }

  void SupervisorService::onQuit () {
    LOG_INFO ("Supervisor actor (", this-> _name, ") was killed");
  }

}
