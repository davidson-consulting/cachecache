#define LOG_LEVEL 10
#define __PROJECT__ "SUPERVISOR"

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
   * ============================================================================================================
   * ============================================================================================================
   * ===========================================      DEPLOYMENT      ===========================================
   * ============================================================================================================
   * ============================================================================================================
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
      Logger::globalInstance ().changeLevel ((*repo)["sys"].getOr ("log-lvl", "info"));
    }

    SupervisorService::__GLOBAL_SYSTEM__ = std::make_shared <actor::ActorSystem> (net::SockAddrV4 (addr, port), 1);
    __GLOBAL_SYSTEM__-> start ();

    LOG_INFO ("Starting service system : ", addr, ":", __GLOBAL_SYSTEM__-> port ());

    __GLOBAL_SYSTEM__-> add <SupervisorService> ("supervisor", repo);

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
   * ============================================================================================================
   * ============================================================================================================
   * ==========================================      CONFIGURE      =============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  SupervisorService::SupervisorService (const std::string & name, actor::ActorSystem * sys, std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg) :
    actor::ActorBase (name, sys)
    , _cfg (cfg)
    , _marketRoutine (0, nullptr)
    , _market (MemorySize::B (0))
    , _memoryPoolSize (MemorySize::B (0))
  {
    LOG_INFO ("Spawning supervisor actor -> ", name);
  }


  void SupervisorService::onStart () {
    this-> configure (this-> _cfg);

    // no need to keep the configuration
    this-> _cfg = nullptr;
  }

  void SupervisorService::configure (std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg) {
    auto & conf = *cfg;

    this-> _memoryPoolSize = MemorySize::GB (1);
    if (conf.contains ("cache")) {
      if (conf ["cache"].contains ("size")) {
        auto unit = conf ["cache"].getOr ("unit", "MB");
        this-> _memoryPoolSize = MemorySize::unit (conf ["cache"]["size"].getI (), unit);
      }
    }

    if (conf.contains ("sys")) {
      this-> _freq = conf["sys"].getOr ("freq", 1.0f);
    } else {
      this-> _freq = 1.0f;
    }

    this-> _market = Market (this-> _memoryPoolSize);
    this-> _running = true;
    this-> _marketRoutine = concurrency::spawn (this, &SupervisorService::marketRoutine);

    LOG_INFO ("Market configured to manage ", this-> _memoryPoolSize.megabytes (), " MB");
  }

  void SupervisorService::onQuit () {
    LOG_INFO ("Supervisor actor (", this-> _name, ") was killed");
    if (this-> _marketRoutine.id != 0) {
      this-> _running = false;
      this-> _marketSleeping.post ();

      concurrency::join (this-> _marketRoutine);
      this-> _marketRoutine = concurrency::Thread (0, nullptr);
    }

    // No need to lock instances mutex, routine thread is already closed
    for (auto & it : this-> _instances) {
      try {
        it.second.remote-> send (config::Dict ()
                                 .insert ("type", RequestIds::POISON_PILL), 1);

        LOG_INFO ("Broadcasting the info to cache entity (", it.first, " aka ", it.second.name, ")");
      } catch (std::runtime_error & e) {
        LOG_ERROR ("Failed to send kill signal to entity (", it.first, " aka ", it.second.name, ") ", e.what ());
      }
    }
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * ========================================      REGISTRATION      ============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  std::shared_ptr<config::ConfigNode> SupervisorService::registerCache (const config::ConfigNode & msg) {
    try {
      auto name = msg ["name"].getStr ();
      auto addr = msg ["addr"].getStr ();
      auto port = msg ["port"].getI ();
      auto unit = static_cast <MemorySize::Unit> (msg.getOr ("unit", (int64_t) MemorySize::Unit::KB));
      auto ask = MemorySize::unit (msg ["size"].getI (), unit);

      auto remote = this-> _system-> remoteActor (name, addr + ":" + std::to_string (port));
      auto uid = this-> _lastUid++;

      {
        WITH_LOCK (this-> _m) {
          LOG_DEBUG ("Lock obtained in register");
          this-> _instances.emplace (uid, CacheInfo {.remote = remote, .name = name});
          this-> _market.registerCache (uid, MemorySize::min (this-> _memoryPoolSize, ask), MemorySize::B (0));
        }
        LOG_DEBUG ("Lock free in register");
      }

      auto resp = std::make_shared <config::Dict> ();
      resp-> insert ("uid", (int64_t) uid);
      resp-> insert ("max-size", (int64_t) this-> _memoryPoolSize.kilobytes ());
      resp-> insert ("unit", (int64_t) MemorySize::Unit::KB);

      LOG_INFO ("Inserted cache : ", uid);
      return ResponseCode (200, resp);
    } catch (std::runtime_error & e) {
      LOG_ERROR ("Error in register : ", e.what ());
      LOG_DEBUG ("Lock free in register");
      return ResponseCode (401);
    }
  }

  void SupervisorService::eraseCache (const config::ConfigNode & msg) {
    try {
      auto uid = msg ["uid"].getI ();
      {
        WITH_LOCK (this-> _m) {
          LOG_DEBUG ("Lock obtained in erase");
          this-> _instances.erase (uid);
          this-> _market.removeCache (uid);
        }
        LOG_DEBUG ("Lock free in erase");
      }

      LOG_INFO ("Erased cache : ", uid);
    }  catch (std::runtime_error & e) {
      LOG_ERROR ("Error in erase : ", e.what ());
    }
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * ===========================================      MESSAGING      ============================================
   * ============================================================================================================
   * ============================================================================================================
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

  /**
   * ============================================================================================================
   * ============================================================================================================
   * =============================================      MARKET      =============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  void SupervisorService::marketRoutine (rd_utils::concurrency::Thread) {
    concurrency::timer t;
    while (this-> _running) {
      t.reset ();
      LOG_INFO ("Market iteration");
      LOG_DEBUG ("Market wait lock");

      WITH_LOCK (this-> _m) { // instances cannot register/quit during a market run
        LOG_DEBUG ("Lock obtained in market");
        this-> updateEntityInfo ();
        this-> _market.run ();
        this-> informEntitySizes ();
      }

      LOG_DEBUG ("Lock free in market");

      auto took = t.time_since_start ();
      auto suppose = 1.0f / this-> _freq;
      if (took < suppose) {
        LOG_INFO ("Sleep ", sleep);
        if (this-> _marketSleeping.wait (suppose - took)) {
          LOG_INFO ("Interupted");
          return;
        }
      }
    }
  }

  void SupervisorService::updateEntityInfo () {
    auto msg = config::Dict ()
      .insert ("type", RequestIds::ENTITY_INFO);

    std::map <uint32_t, CacheInfo> rests;
    for (auto & inst : this-> _instances) {
      try {
        auto res = inst.second.remote-> request (msg, 1); // 1 seconds timeout
        if (res == nullptr || res-> getOr ("code", 0) != 200) {
          throw std::runtime_error ("Cache request failure");
        }

        auto unit = static_cast <MemorySize::Unit> (msg.getOr ("unit", (int64_t) MemorySize::Unit::KB));
        auto usage = MemorySize::unit ((*res)["content"]["usage"].getI (), unit);
        LOG_INFO ("Current usage of (", inst.first, " aka ", inst.second.name, ") = ", usage.megabytes (), " MB");
        rests.emplace (inst.first, CacheInfo {.remote = std::move (inst.second.remote),
                                              .name = inst.second.name,
                                              .uid = inst.second.uid});

        this-> _market.updateUsage (inst.first, usage);
      } catch (const std::runtime_error & e) {
        LOG_ERROR ("Cache (", inst.first, " aka ", inst.second.name, ") refused to answer information update, [", e.what (), "]");
        LOG_INFO ("Killing cache (", inst.first, " aka ", inst.second.name, ")");
        try {
          inst.second.remote-> send (config::Dict ()
                                     .insert ("type", RequestIds::POISON_PILL), 1);
        } catch (const std::runtime_error & e) {
          LOG_ERROR ("Failed to send kill signal to entity (", inst.first, " aka ", inst.second.name, ") ", e.what ());
        }

        this-> _market.removeCache (inst.first);
      }
    }

    this-> _instances = std::move (rests);
  }

  void SupervisorService::informEntitySizes () {
    std::map <uint32_t, CacheInfo> rests;
    for (auto & inst : this-> _instances) {
      try {
        if (this-> _market.hasChanged (inst.first)) {
          auto newSize = this-> _market.getCacheSize (inst.first);        
          inst.second.remote-> send (config::Dict ()
                                     .insert ("type", RequestIds::UPDATE_SIZE)
                                     .insert ("size", (int64_t) newSize.kilobytes ())
                                     .insert ("unit", (int64_t) MemorySize::Unit::KB), 1);

          LOG_INFO ("Set size of (", inst.first, " aka ", inst.second.name, ") = ", newSize.megabytes (), " MB");
        } else {
          LOG_INFO ("Cache (", inst.first, " aka ", inst.second.name, ") does not need an update");
        }

        rests.emplace (inst.first, CacheInfo {.remote = std::move (inst.second.remote),
                                                .name = inst.second.name,
                                                .uid = inst.second.uid});
      } catch (...) {
        LOG_ERROR ("Cache (", inst.first, " aka ", inst.second.name, ") was disconnected during size update");
        LOG_INFO ("Killing cache (", inst.first, " aka ", inst.second.name, ")");
        this-> _market.removeCache (inst.first);
      }    
    }

    this-> _instances = std::move (rests);
  }


}
