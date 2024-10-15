#define LOG_LEVEL 10

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
    if (repo-> contains ("sys")) {
      addr = (*repo) ["sys"].getOr ("addr", "0.0.0.0");
      port = (*repo) ["sys"].getOr ("port", 0);
    }

    __GLOBAL_SYSTEM__ = std::make_shared <actor::ActorSystem> (net::SockAddrV4 (addr, port), 2);
    __GLOBAL_SYSTEM__-> start ();

    LOG_INFO ("Starting service system : ", addr, ":", __GLOBAL_SYSTEM__-> port ());

    __GLOBAL_SYSTEM__-> add <CacheService> ("instance", repo);

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


  CacheService::CacheService (const std::string & name, actor::ActorSystem * sys, const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg) :
    actor::ActorBase (name, sys)
  {
    LOG_INFO ("Spawning a new cache instance -> ", name);
    this-> _cfg = cfg;
  }

  void CacheService::onStart () {
    this-> connectSupervisor (this-> _cfg);
    this-> configureServer (this-> _cfg);

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
    LOG_INFO ("Cache instance (", this-> _name, ") received a request : ", type);

    return ResponseCode (404);
  }

  void CacheService::onQuit () {
    LOG_INFO ("Killing cache instance -> ", this-> _name);
    if (this-> _supervisor != nullptr) {
      this-> _supervisor-> send (config::Dict ()
                                 .insert ("type", RequestIds::EXIT)
                                 .insert ("uid", (int64_t) this-> _uid));
    }
    ::exit (0);
  }

  /**
   * ==========================================================================
   * ==========================================================================
   * =========================     SUPERVISOR     =============================
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
        .insert ("port", (int64_t) this-> _system-> port ());

      auto resp = this-> _supervisor-> request (msg);
      if (resp == nullptr || resp-> getOr ("code", 0) != 200) {
        throw std::runtime_error ("Failed to register : " + this-> _name);
      } else {
        this-> _uid = (*resp) ["content"]["uid"].getI ();
      }

    } catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
      throw std::runtime_error ("Failed to connect to supervisor");
    }

    LOG_INFO ("Connected to supervisor");
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

      this-> _clients = std::make_shared <net::TcpServer> (net::SockAddrV4 (sAddr, sPort), nbThreads, maxCon);
      this-> _clients-> start (this, &CacheService::onClient);
    }  catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
      throw std::runtime_error ("Failed to configure tcp server");
    }

    LOG_INFO ("Cache tcp server ready on port ", this-> _clients-> port ());
  }

  void CacheService::onClient (rd_utils::net::TcpSessionKind kind, std::shared_ptr <net::TcpSession> session) {
    uint32_t id = (*session)-> receiveI32 ();
    if (id == 's') {
      this-> onSet (session);
    } else if (id == 'g') {
      this-> onGet (session);
    }
  }

  void CacheService::onSet (std::shared_ptr <net::TcpSession> session) {
    uint32_t keyLen = (*session)-> receiveI32 ();
    uint32_t valLen = (*session)-> receiveI32 ();

    if (keyLen > Limits::MAX_KEY) {
      LOG_ERROR ("Key too large : ", keyLen);
      (*session)-> close ();
      return;
    }

    if (valLen > Limits::MAX_VALUE) {
      LOG_ERROR ("Value too large : ", valLen);
      (*session)-> close ();
      return;
    }

    std::string key = this-> readStr (*session, keyLen);
    std::string value = this-> readStr (*session, valLen);

    std::cout << "Set a key : " << key << " " << value << std::endl;

  }

  void CacheService::onGet (std::shared_ptr <net::TcpSession> session) {
    uint32_t keyLen = (*session)-> receiveI32 ();
    if (keyLen > Limits::MAX_KEY) {
      LOG_ERROR ("Key too large : ", keyLen);
      (*session)-> close ();
      return;
    }

    std::string key = this-> readStr (*session, keyLen);
    std::cout << "Get the key : " << key << std::endl;
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
