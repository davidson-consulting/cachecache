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
    uint32_t port = 0;
    int64_t nbThreads = 1;
    uint32_t nbInstances = 1;
    if (repo-> contains ("sys")) {
      addr = (*repo) ["sys"].getOr ("addr", "0.0.0.0");
      port = (*repo) ["sys"].getOr ("port", 0);
      nbThreads = (*repo) ["sys"].getOr ("nb-threads", 1);
      nbInstances = (*repo)["sys"].getOr ("instances", 1);

      Logger::globalInstance ().changeLevel ((*repo)["sys"].getOr ("log-lvl", "info"));
    }

    if (nbThreads == 0) nbThreads = 1;

    __GLOBAL_SYSTEM__ = std::make_shared <actor::ActorSystem> (net::SockAddrV4 (addr, port), nbThreads);

    __GLOBAL_SYSTEM__-> joinOnEmpty (true);
    __GLOBAL_SYSTEM__-> start ();

    LOG_INFO ("Starting service system : ", addr, ":", __GLOBAL_SYSTEM__-> port ());

    for (uint32_t i = 0 ; i < nbInstances ; i++) {
      __GLOBAL_SYSTEM__-> add <CacheService> ("instance(" + std::to_string (i) + ")", repo, i);
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
  {
    LOG_INFO ("Spawning a new cache instance -> ", name);
    this-> _cfg = cfg;
    this-> _uniqNb = nb;
  }

  void CacheService::onStart () {
    this-> _fullyConfigured = false;
    if (this-> connectSupervisor (this-> _cfg)) {
      this-> configureEntity (this-> _cfg);
      this-> configureServer (this-> _cfg);
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
    case RequestIds::UPDATE_SIZE :
      this-> onSizeUpdate (msg);
      break;
    case RequestIds::POISON_PILL :
      this-> _fullyConfigured = false;
      this-> _supervisor = nullptr;
      this-> exit ();
      break;
    default:
      LOG_ERROR ("Unkown message");
      break;
    }
  }

  std::shared_ptr<config::ConfigNode> CacheService::onRequest (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", RequestIds::NONE);
    switch (type) {
    case RequestIds::ENTITY_INFO :
      return this-> onEntityInfoRequest (msg);
    }

    return ResponseCode (404);
  }

  void CacheService::onQuit () {
    this-> _fullyConfigured = false;
    if (this-> _supervisor != nullptr) {
      try {
        this-> _supervisor-> send (config::Dict ()
                                   .insert ("type", RequestIds::EXIT)
                                   .insert ("uid", (int64_t) this-> _uid));
      } catch (...) {
        LOG_WARN ("Supervisor is offline.");
      }
    }

    LOG_INFO ("Killing cache instance -> ", this-> _name);
    if (this-> _entity != nullptr) {
      this-> _entity-> dispose ();
      this-> _entity = nullptr;
    }
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * ========================     MARKET ROUTINE     ==========================
   * ==========================================================================
   * ==========================================================================
   */

  void CacheService::onSizeUpdate (const config::ConfigNode & msg) {
    if (this-> _fullyConfigured) { // entity must be running to be resized
      auto unit = static_cast <MemorySize::Unit> (msg.getOr ("unit", (int64_t) MemorySize::Unit::KB));
      auto size = MemorySize::unit (msg ["size"].getI (), unit);
      this-> _entity-> resize (size);
    }
  }

  std::shared_ptr <config::ConfigNode> CacheService::onEntityInfoRequest (const config::ConfigNode & msg) {
    auto result = std::make_shared <config::Dict> ();
    if (this-> _fullyConfigured && this-> _entity != nullptr) { // entity must be running to have a size
      result-> insert ("usage", (int64_t) this-> _entity-> getCurrentMemoryUsage ().kilobytes ());
    } else {
      result-> insert ("usage", (int64_t) 0);
    }

    result-> insert ("unit", (int64_t) MemorySize::Unit::KB);
    return ResponseCode (200, result);
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * =========================     SUPERVISOR     =============================
   * ==========================================================================
   * ==========================================================================
   */

  bool CacheService::connectSupervisor (const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg) {
    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> act = nullptr;
    try {
      auto & conf = *cfg;
      std::string sName = "supervisor", sAddr = "127.0.0.1";
      int64_t sPort = 8080, size = 1024 * 1024;

      if (conf.contains ("supervisor")) {
        sName = conf ["supervisor"].getOr ("name", sName);
        sAddr = conf ["supervisor"].getOr ("addr", sAddr);
        sPort = conf ["supervisor"].getOr ("port", sPort);
      }

      if (conf.contains ("cache")) {
        size = conf ["cache"].getOr ("size", 1024) * 1024;
      }

      std::string iface = "lo";
      if (conf.contains ("sys")) {
        iface = conf ["sys"].getOr ("iface", "lo");
      } else iface = "lo";

      this-> _supervisor = this-> _system-> remoteActor (sName, sAddr + ":" + std::to_string (sPort), false);
      this-> _ifaceAddr = rd_utils::net::Ipv4Address::getIfaceIp (iface).toString ();

      auto msg = config::Dict ()
        .insert ("type", RequestIds::REGISTER)
        .insert ("name", this-> _name)
        .insert ("addr", this-> _ifaceAddr)
        .insert ("port", (int64_t) this-> _system-> port ())
        .insert ("size", size);

      auto resp = this-> _supervisor-> request (msg, 5); // 5 seconds timeout
      if (resp == nullptr || resp-> getOr ("code", 0) != 200) {
        LOG_ERROR ("Failure");
        throw std::runtime_error ("Failed to register : " + this-> _name);
      } else {
        LOG_INFO ("Server response : ", *resp);
        this-> _uid = (*resp) ["content"]["uid"].getI ();
        auto unit = static_cast <MemorySize::Unit> ((*resp)["content"].getOr ("unit", (int64_t) MemorySize::Unit::KB));
        this-> _regSize = MemorySize::unit ((*resp)["content"]["max-size"].getI (), unit);
      }

    } catch (std::runtime_error & err) {
      LOG_ERROR ("Failed to connect to supervisor because, ", err.what ());
      this-> exit ();
      return false;
    }

    LOG_INFO ("Connected to supervisor");
    return true;
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
      int64_t nbThreads = 1, maxCon = -1, sPort = 0;

      if (conf.contains ("service")) {
        sAddr = conf ["service"].getOr ("addr", sAddr);
        sPort = conf ["service"].getOr ("port", sPort);
        nbThreads = conf ["service"].getOr ("nb-threads", nbThreads);
        maxCon = conf ["service"].getOr ("max-conn", maxCon);
      }

      if (sPort != 0) sPort += this-> _uniqNb;

      this-> _clients = std::make_shared <net::TcpServer> (net::SockAddrV4 (sAddr, sPort), nbThreads, maxCon);
      this-> _clients-> start (this, &CacheService::onClient);
    }  catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
      throw std::runtime_error ("Failed to configure tcp server");
    }

    LOG_INFO ("Cache tcp server ready on port ", this-> _clients-> port ());
  }

  void CacheService::onClient (rd_utils::net::TcpSessionKind kind, std::shared_ptr <net::TcpSession> session) {
    try {
      uint32_t id = (*session)-> receiveI32 ();
      if (id == 's') {
        this-> onSet (*session);
      } else if (id == 'g') {
        this-> onGet (*session);
      }
    } catch (const std::runtime_error & e) {
      LOG_ERROR ("Client connection reset : ", e.what ());
      (*session)-> close ();
    }
  }

  void CacheService::onSet (net::TcpSession & session) {
    uint32_t keyLen = session-> receiveI32 ();

    if (keyLen > Limits::MAX_KEY) {
      LOG_ERROR ("Key too large : ", keyLen);
      session-> close ();
      return;
    }

    std::string key = this-> readStr (session, keyLen);
    this-> _entity-> insert (key, session);
  }

  void CacheService::onGet (net::TcpSession & session) {
    uint32_t keyLen = session-> receiveI32 ();
    if (keyLen > Limits::MAX_KEY) {
      LOG_ERROR ("Key too large : ", keyLen);
      session-> close ();
      return;
    }

    std::string key = this-> readStr (session, keyLen);
    this-> _entity-> find (key, session);
  }

  std::string CacheService::readStr (net::TcpSession & session, uint32_t resLen) {
    std::stringstream ss;
    char buffer [1024];
    while (resLen > 0) {
      uint32_t rest = std::min ((uint32_t) 1024, resLen);
      if (!session-> receiveRaw<char> (buffer, rest)) {
        return "";
      }

      ss << buffer;
      resLen -= rest;
    }

    return ss.str ();
  }
}
