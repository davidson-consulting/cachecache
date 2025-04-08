#define LOG_LEVEL 10
#define __PROJECT__ "CACHE"

#include <csignal>
#include "service.hh"
#include <rd_utils/foreign/CLI11.hh>

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;

using namespace cachecache::utils;

namespace cachecache::instance {

  std::shared_ptr <actor::ActorSystem> CacheService::__GLOBAL_SYSTEM__ = nullptr;

  /**
   * ========================================================================
   * ========================================================================
   * =======================    DEPLOYEMENT   ===============================
   * ========================================================================
   * ========================================================================
   */

  void CacheService::deploy (int argc, char ** argv) {

    ::signal (SIGINT, &ctrlCHandler);
    ::signal (SIGTERM, &ctrlCHandler);
    ::signal (SIGKILL, &ctrlCHandler);

    LOG_INFO ("Starting deployement of a Cache actor");
    auto configFile = CacheService::appOption (argc, argv);

    auto repo = toml::parseFile (configFile);
    std::string addr = "0.0.0.0";
    std::string name = "instance";

    uint32_t port = 0;
    int64_t nbThreads = 1;
    uint32_t nbInstances = 1;
    if (repo-> contains ("sys")) {
      addr = (*repo) ["sys"].getOr ("addr", "0.0.0.0");
      port = (*repo) ["sys"].getOr ("port", 0);
      if ((*repo)["sys"].contains ("nb-threads")) {
        nbThreads = (*repo) ["sys"].getOr ("nb-threads", 1);
      }

      if ((*repo)["sys"].contains ("instances")) {
        nbInstances = (*repo)["sys"].getOr ("instances", 1);
      }

      name = (*repo)["sys"].getOr ("name", name);
      Logger::globalInstance ().changeLevel ((*repo)["sys"].getOr ("log-lvl", "info"));
    }

    if (nbThreads == 0) nbThreads = 1;

    __GLOBAL_SYSTEM__ = std::make_shared <actor::ActorSystem> (net::SockAddrV4 (addr, port), nbThreads);

    __GLOBAL_SYSTEM__-> joinOnEmpty (true);
    __GLOBAL_SYSTEM__-> start ();

    LOG_INFO ("Starting service system : ", addr, ":", __GLOBAL_SYSTEM__-> port ());
    try {
      if (nbInstances != 1) {
        for (uint32_t i = 0 ; i < nbInstances ; i++) {
          __GLOBAL_SYSTEM__-> add <CacheService> (name + "_" + std::to_string (i), repo, i);
        }
      } else {
        __GLOBAL_SYSTEM__-> add <CacheService> (name, repo, 0);
      }
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("Killing actor system: ", err.what ());
      LOG_ERROR ("Aborting.");

      __GLOBAL_SYSTEM__-> dispose ();
      return;
    }

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

  void CacheService::ctrlCHandler (int signum) {
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


  CacheService::CacheService (const std::string & name, actor::ActorSystem * sys, const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg, uint32_t nb) :
    actor::ActorBase (name, sys)
    , _regSize (MemorySize::B (0))
    , _traceRoutine (0)
  {
    LOG_INFO ("Spawning a new cache instance -> ", name);
    this-> _cfg = cfg;
    this-> _uniqNb = nb;
  }

  void CacheService::onStart () {
    this-> _fullyConfigured = false;
    try {
      this-> configureEntity (this-> _cfg);
      this-> configureServer (this-> _cfg);
    } catch (const std::runtime_error & err) {
      this-> onQuit ();
      throw err;
    }

    // no need to keep the configuration
    this-> _cfg = nullptr;
    this-> _fullyConfigured = true;
  }

  void CacheService::configureEntity (const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg) {
    try {
      auto & conf = *cfg;
      std::string name = "cache";
      int64_t ttl = Limits::BASE_TTL;

      if (conf.contains ("cache")) {
        name = conf ["cache"].getOr ("name", name);
        ttl = conf ["cache"].getOr ("ttl", ttl);
        CONF_LET (unit, conf ["cache"]["unit"].getStr (), std::string ("MB"));
        this-> _regSize = MemorySize::unit (conf ["cache"].getOr ("size", 1024), unit);
      } else {
        this-> _regSize = MemorySize::MB (1024);
      }

      this-> _entity = std::make_shared <CacheEntity> ();
      this-> _entity-> configure (name, this-> _regSize, ttl);
    } catch (std::runtime_error & e) {
      LOG_ERROR ("Cache entity configuration failed (", e.what (), "). Abort");
      throw std::runtime_error ("in entity configuration");
    }
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
      this-> _fullyConfigured = false;
      this-> exit ();
      break;
    default:
      LOG_ERROR ("Unkown message");
      break;
    }
  }

  void CacheService::onQuit () {
    this-> _fullyConfigured = false;

    if (this-> _clients != nullptr) {
      this-> _clients-> stop ();
      this-> _clients-> join ();
      this-> _clients = nullptr;
    }

    if (this-> _traceRoutine.id != 0) {
      this-> _running = false;
      this-> _traceSleeping.post ();

      concurrency::join (this-> _traceRoutine);
      this-> _traceRoutine = concurrency::Thread (0);
    }

    LOG_INFO ("Killing cache instance -> ", this-> _name);
    if (this-> _entity != nullptr) {
      WITH_LOCK (this-> _m) {
        this-> _entity-> dispose ();
        this-> _entity = nullptr;
      }
    }
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * ===========================     SERVER     ===============================
   * ==========================================================================
   * ==========================================================================
   */

  void CacheService::configureServer (const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg) {
    try {
      auto & conf = *cfg;

      std::string sAddr = "0.0.0.0";
      int64_t nbThreads = -1, sPort = 0;

      if (conf.contains ("service")) {
        sAddr = conf ["service"].getOr ("addr", sAddr);
        sPort = conf ["service"].getOr ("port", sPort);
        nbThreads = conf ["service"].getOr ("nb-threads", nbThreads);
      }

      if (sPort != 0) sPort += this-> _uniqNb;
      if (conf.contains ("sys")) {
        this-> _freq = conf["sys"].getOr ("freq", 1.0f);
        auto filename = conf ["sys"].getOr ("export-traces", "/tmp/entity") + "." + std::to_string (sPort) + ".json";
        this-> _traces = std::make_shared <rd_utils::utils::trace::JsonExporter> (filename);
        this-> _traceRoutine = concurrency::spawn (this, &CacheService::traceRoutine);
      }

      this-> _clients = std::make_shared <net::TcpServer> (net::SockAddrV4 (sAddr, sPort), nbThreads);
      this-> _clients-> start (this, &CacheService::onClient);

      if (sPort == 0) {
        rd_utils::utils::write_file ("entity_port." + std::to_string (this-> _uniqNb) + "." + this-> _name, std::to_string (this-> _clients-> port ()));
      }

    }  catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
      throw std::runtime_error ("Failed to configure tcp server");
    }

    LOG_INFO ("Cache tcp server ready on port ", this-> _clients-> port ());
  }

  void CacheService::onClient (std::shared_ptr <net::TcpStream> session) {
    if (!this-> _fullyConfigured) {
      session-> close ();
      return;
    }

    try {
      uint32_t id = session-> receiveI32 ();
      if (id == 's') {
        this-> onSet (*session);
      } else if (id == 'g') {
        this-> onGet (*session);
      }
    } catch (const std::runtime_error & e) {
      LOG_ERROR ("Client connection reset : ", e.what ());
      session-> close ();
    }
  }

  void CacheService::onSet (net::TcpStream& session) {
    rd_utils::concurrency::timer t;
    uint32_t keyLen = session.receiveU32 ();

    if (keyLen > Limits::MAX_KEY) {
      LOG_ERROR ("Key too large : ", keyLen);
      session.close ();
      return;
    }

    std::string key = this-> readStr (session, keyLen);
    WITH_LOCK (this-> _m) {
      this-> _entity-> insert (key, session);
      this-> _set += 1;
    }
  }

  void CacheService::onGet (net::TcpStream& session) {
    rd_utils::concurrency::timer t;
    uint32_t keyLen = session.receiveU32 ();
    if (keyLen > Limits::MAX_KEY) {
      LOG_ERROR ("Key too large : ", keyLen);
      session.close ();
      return;
    }

    std::string key = this-> readStr (session, keyLen);
    WITH_LOCK (this-> _m) {
      if (this-> _entity-> find (key, session)) {
        this-> _hit += 1;
      } else {
        this-> _miss += 1;
      }
    }
  }

  std::string CacheService::readStr (net::TcpStream& stream, uint32_t resLen) {
    std::stringstream ss;
    char buffer [1024];
    while (resLen > 0) {
      uint32_t rest = std::min ((uint32_t) 1023, resLen);
      if (!stream.receiveRaw <char> (buffer, rest)) {
        return "";
      }

      buffer [rest] = 0;
      ss << buffer;
      resLen -= rest;
    }

    return ss.str ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          TRACES          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void CacheService::traceRoutine (rd_utils::concurrency::Thread) {
    concurrency::timer t;
    this-> _running = true;

    while (this-> _running) {
      t.reset ();

      WITH_LOCK (this-> _m) { // instances cannot register/quit during a market run
        config::Dict d;
        d.insert ("hit", this-> _hit);
        d.insert ("miss", this-> _miss);
        d.insert ("set", this-> _set);
        d.insert ("usage", (int64_t) this-> _entity-> getCurrentMemoryUsage ().kilobytes ());
        d.insert ("size", (int64_t) this-> _entity-> getSize ().kilobytes ());
        d.insert ("unit", "KB");

        this-> _traces-> append (time (NULL), d);

        this-> _hit = 0;
        this-> _miss = 0;
        this-> _set = 0;
      }

      auto took = t.time_since_start ();
      auto suppose = 1.0f / this-> _freq;
      if (took < suppose) {
        if (this-> _traceSleeping.wait (suppose - took)) {
          LOG_INFO ("Interupted");
          return;
        }
      }
    }
  }


}
