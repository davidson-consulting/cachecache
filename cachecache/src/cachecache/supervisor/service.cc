#define LOG_LEVEL 10

#include <csignal>
#include "service.hh"

#include <cachecache/instance/service.hh>
#include <rd_utils/foreign/CLI11.hh>


using namespace rd_utils;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;

using namespace cachecache::utils;

namespace cachecache::supervisor {

  std::shared_ptr <actor::ActorSystem> SupervisorService::__GLOBAL_SYSTEM__ = nullptr;

  /**
   * ========================================================================
   * ========================================================================
   * =======================    DEPLOYEMENT   ===============================
   * ========================================================================
   * ========================================================================
   */

  void SupervisorService::deploy (int argc, char ** argv) {

    ::signal (SIGINT, &ctrlCHandler);
    ::signal (SIGTERM, &ctrlCHandler);
    ::signal (SIGKILL, &ctrlCHandler);

    LOG_INFO ("Starting deployement of a Supervisor actor");
    auto configFile = SupervisorService::appOption (argc, argv);

    auto repo = toml::parseFile (configFile);
    std::string addr = "0.0.0.0";
    uint32_t port = 0;
    if (repo-> contains ("sys")) {
      addr = (*repo) ["sys"].getOr ("addr", "0.0.0.0");
      port = (*repo) ["sys"].getOr ("port", 0);
    }

    SupervisorService::__GLOBAL_SYSTEM__ = std::make_shared <actor::ActorSystem> (net::SockAddrV4 (addr, port), 1);
    __GLOBAL_SYSTEM__-> start ();

    LOG_INFO ("Starting service system : ", addr, ":", __GLOBAL_SYSTEM__-> port ());

    config::Dict config ;
    if (repo-> contains ("supervisor")) {
      config.insert ("config", repo-> get ("supervisor"));
    }

    __GLOBAL_SYSTEM__-> add <SupervisorService> ("supervisor", config);

    __GLOBAL_SYSTEM__-> join ();
    __GLOBAL_SYSTEM__-> dispose ();
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

  void SupervisorService::ctrlCHandler (int signum) {
    LOG_INFO ("Signal ", strsignal(signum), " received");
    if (__GLOBAL_SYSTEM__ != nullptr) {
      auto aux = __GLOBAL_SYSTEM__;
      __GLOBAL_SYSTEM__ = nullptr;
      aux-> dispose ();
    }

    ::exit (0);
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
  }

  std::shared_ptr<config::ConfigNode> SupervisorService::registerCache (const config::ConfigNode & msg) {
    try {
      auto name = msg ["name"].getStr ();
      auto addr = msg ["addr"].getStr ();
      auto port = msg ["port"].getI ();

      auto remote = this-> _system-> remoteActor (name, addr + ":" + std::to_string (port));
      auto uid = this-> _lastUid++;

      this-> _instances.emplace (uid, CacheInfo {.remote = remote, .name = name});
      auto resp = std::make_shared <config::Dict> ();
      resp-> insert ("uid", (int64_t) uid);

      LOG_INFO ("Inserted cache : ", uid);
      return ResponseCode (200, resp);
    } catch (std::runtime_error & e) {
      LOG_ERROR ("Error in register : ", e.what ());
      return ResponseCode (401);
    }
  }

  void SupervisorService::eraseCache (const config::ConfigNode & msg) {
    try {
      auto uid = msg ["uid"].getI ();
      this-> _instances.erase (uid);
      LOG_INFO ("Erased cache : ", uid);
    }  catch (std::runtime_error & e) {
      LOG_ERROR ("Error in erase : ", e.what ());
    }
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * =========================     MESSAGING     ==============================
   * ==========================================================================
   * ==========================================================================
   */

  void SupervisorService::onMessage (const rd_utils::utils::config::ConfigNode & msg) {
    auto type = msg.getOr ("type", RequestIds::NONE);
    LOG_INFO ("Supervisor actor (", this-> _name, ") received a message : ", type);
    switch (type) {
      case RequestIds::EXIT :
        this-> eraseCache (msg);
        break;
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> SupervisorService::onRequest (const rd_utils::utils::config::ConfigNode & msg) {
    auto type = msg.getOr ("type", RequestIds::NONE);
    LOG_INFO ("Supervisor actor (", this-> _name, ") received a request : ", type);
    switch (type) {
    case RequestIds::REGISTER :
      return this-> registerCache (msg);
    }

    return ResponseCode (404);
  }

  void SupervisorService::onQuit () {
    LOG_INFO ("Supervisor actor (", this-> _name, ") was killed");
    for (auto & it : this-> _instances) {
      it.second.remote-> send (config::Dict ()
                               .insert ("type", RequestIds::POISON_PILL));

      LOG_INFO ("Broadcasting the info to cache entity : ", it.first);
    }
  }

}
