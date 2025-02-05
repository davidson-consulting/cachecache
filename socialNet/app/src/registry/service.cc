#define __PROJECT__ "REGISTRY"

#include "service.hh"

using namespace rd_utils;
using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet {

  RegistryService::RegistryService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode&) :
    actor::ActorBase (name, sys, false) // registry is not atomic
  {}

  void RegistryService::onQuit () {
    auto msg = config::Dict ()
      .insert ("type", RequestCode::POISON_PILL);

    WITH_LOCK (this-> getMutex ()) {
      // Killing non order services
      for (auto & it : this-> __services__) {
        for (auto & jt : it.second.second) {
          auto act = this-> _system-> remoteActor (jt.id, net::SockAddrV4 (jt.addr, jt.port));
          if (act != nullptr) {
            LOG_INFO ("Killing : ", jt.id);
            act-> send (msg);
          }
        }
      }
    }
  }


  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          REQUESTS          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<rd_utils::utils::config::ConfigNode> RegistryService::onRequest (const rd_utils::utils::config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::REGISTER_SERVICE) {
        return this-> registerService (msg);
      }

      elof_v (RequestCode::CLOSE_SERVICE) {
        return this-> closeService (msg);
      }

      elof_v (RequestCode::FIND) {
        return this-> findService (msg);
      }

      elfo {
        return response (ResponseCode::NOT_FOUND);
      }
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> RegistryService::registerService (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto kind = msg ["kind"].getStr ();
      auto id = msg ["id"].getStr ();
      auto addr = msg ["addr"].getStr ();
      uint32_t port = msg ["port"].getI ();

      LOG_INFO ("Registering service : ", kind, " ", id, " ", addr, " ", port);
      WITH_LOCK (this-> getMutex ()) {
        auto it = this-> __services__.find (kind);
        if (it == this-> __services__.end ()) {
          this-> __services__.emplace (kind, std::make_pair<int, std::vector <Service> > (0, {Service {.id = id, .addr = addr, .port = port}}));
        } else {
          it-> second.second.push_back ({.id = id, .addr = addr, .port = port});
        }
      }

      return response (ResponseCode::OK);
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("RegistryService::registerService ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> RegistryService::findService (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto kind = msg ["kind"].getStr ();
      LOG_DEBUG ("Find service : ", msg);
      Service src;
      WITH_LOCK (this-> getMutex ()) {
        auto it = this-> __services__.find (kind);
        if (it != this-> __services__.end ()) {
          auto index = it-> second.first;
          it-> second.first += 1;
          if (it-> second.first >= it-> second.second.size ()) {
            it-> second.first = 0;
          }
          src = it-> second.second [index];
        } else {
          LOG_WARN ("No service : ", kind);
          return response (ResponseCode::NOT_FOUND);
        }
      }

      LOG_DEBUG ("Found service : ", kind);
      auto res = config::Dict ()
        .insert ("addr", std::make_shared <config::String> (src.addr))
        .insert ("id", std::make_shared <config::String> (src.id))
        .insert ("port", std::make_shared <config::Int> (src.port));

      return response (ResponseCode::OK, res);
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("RegistryService::findService ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> RegistryService::closeService (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto kind = msg ["kind"].getStr ();
      auto id = msg ["id"].getStr ();
      auto addr = msg ["addr"].getStr ();
      uint32_t port = msg ["port"].getI ();

      LOG_INFO ("Closing service : ", kind, " ", id, " ", addr, " ", port);

      WITH_LOCK (this-> getMutex ()) {
        auto it = this-> __services__.find (kind);
        if (it != this-> __services__.end ()) {
          it-> second.first = 0;
          std::vector <Service> res;
          for (auto & jt : it-> second.second) {
            if (jt.addr != addr || jt.id != id || jt.port != port) {
              res.push_back (jt);
            }
          }

          if (res.size () == 0) {
            this-> __services__.erase (kind);
          } else {
            it-> second.second = res;
          }
        }
      }

      return response (ResponseCode::OK);
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("RegistryService::closeService ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          CONNECTING          ===================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <rd_utils::concurrency::actor::ActorRef> connectRegistry (rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode & conf) {
    try {
      std::shared_ptr <rd_utils::concurrency::actor::ActorRef> act = nullptr;

      auto name = conf ["registry"].getOr ("name", "registry");
      auto addr = conf ["registry"].getOr ("addr", "127.0.0.1");
      uint32_t port = conf ["registry"].getOr ("port", 0);

      if (port == 0) {
        port = std::atoi (rd_utils::utils::read_file ("./registry_port").c_str ());
      }

      act = sys-> remoteActor (name, net::SockAddrV4 (addr, port));
      if (act == nullptr) {
        throw std::runtime_error ("Failed to connect to registry");
      }

      return act;
    } catch (std::runtime_error & err) {
      LOG_ERROR ("connectRegistry ", err.what ());
      throw std::runtime_error ("Failed to connect to registry");
    }
  }

  void registerService (std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind, const std::string & name, uint32_t port, const std::string & iface) {
    try {
      std::string addr = rd_utils::net::Ipv4Address::getIfaceIp (iface).toString ();
      auto req = config::Dict ()
        .insert ("type", RequestCode::REGISTER_SERVICE)
        .insert ("kind", kind)
        .insert ("id", name)
        .insert ("addr", addr)
        .insert ("port", (int64_t) port);

      LOG_INFO ("Registering to registry : ", kind, " ", name, " ", addr, " ", port);

      auto resp = registry-> request (req).wait ();
      if (resp == nullptr || resp-> getOr ("code", 0) != ResponseCode::OK) {
        throw std::runtime_error ("Failed to register service : " + kind + "/" + name);
      }
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("registerService ", err.what ());
      throw std::runtime_error ("Failed to register service : " + kind + "/" + name);
    }
  }

  std::shared_ptr <rd_utils::concurrency::actor::ActorRef> findService (rd_utils::concurrency::actor::ActorSystem * sys, std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind) {
    uint32_t nbTries = 10;
    for (uint32_t i = 0 ; i < nbTries ; i++) {
      try {
        auto req = config::Dict ()
          .insert ("type", RequestCode::FIND)
          .insert ("kind", kind);

        auto resp = registry-> request (req, 20).wait ();
        if (resp != nullptr || resp-> getOr ("code", 0) == ResponseCode::OK) {
          auto addr = (*resp) ["content"]["addr"].getStr ();
          auto id = (*resp) ["content"]["id"].getStr ();
          uint32_t port = (*resp) ["content"]["port"].getI ();

          return sys-> remoteActor (id, net::SockAddrV4 (addr, port));
        } else {
          if (resp != nullptr) std::cout << "Failure resp : " << *resp << std::endl;
          else std::cout << "Failure " << req << std::endl;
        }
      } catch (...) {}
    }

    return nullptr;
  }

  void closeService (std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind, const std::string & name, uint32_t port, const std::string & iface) {
    try {
      auto addr = rd_utils::net::Ipv4Address::getIfaceIp (iface).toString ();
      auto req = config::Dict ()
        .insert ("type", RequestCode::CLOSE_SERVICE)
        .insert ("kind", kind)
        .insert ("id", name)
        .insert ("addr", addr)
        .insert ("port", (int64_t) port);

      LOG_INFO ("Closing to registry : ", kind, " ", name, " ", addr, " ", port);

      auto resp = registry-> request (req).wait ();
      if (resp == nullptr || resp-> getOr ("code", 0) != 200) {
        throw std::runtime_error (std::string ("Failed to close service : ") + kind + std::string ("/") + name);
      }
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("closeService ", err.what ());
      throw std::runtime_error (std::string ("Failed to close service : ") + kind + std::string ("/") + name);
    }
  }

}
