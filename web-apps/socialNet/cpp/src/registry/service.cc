#define LOG_LEVEL 10
#define __PROJECT__ "REGISTRY"

#include "service.hh"

using namespace rd_utils;
using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet {

  RegistryService::RegistryService (const std::string & name, actor::ActorSystem * sys) :
    actor::ActorBase (name, sys)
  {}

  std::shared_ptr<rd_utils::utils::config::ConfigNode> RegistryService::onRequest (const rd_utils::utils::config::ConfigNode & msg) {
    auto req = msg.getOr ("type", "none");
    if (req == "register") {
      return this-> registerService (msg);
    } else if (req == "close") {
      return this-> closeService (msg);
    } else if (req == "get") {
      return this-> findService (msg);
    } else {
      return ResponseCode (404);
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> RegistryService::registerService (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto kind = msg ["kind"].getStr ();
      auto id = msg ["id"].getStr ();
      auto addr = msg ["addr"].getStr ();
      uint32_t port = msg ["port"].getI ();

      LOG_INFO ("Registering service : ", kind, " ", id, " ", addr, " ", port);
      auto it = this-> _services.find (kind);
      if (it == this-> _services.end ()) {
        this-> _services.emplace (kind, std::make_pair<int, std::vector <Service> > (0, {Service {.id = id, .addr = addr, .port = port}}));
      } else {
        it-> second.second.push_back ({.id = id, .addr = addr, .port = port});
      }

      return ResponseCode (200);
    } catch (...) {}

    return ResponseCode (400);
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> RegistryService::findService (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto kind = msg ["kind"].getStr ();
      auto it = this-> _services.find (kind);
      if (it != this-> _services.end ()) {
        auto index = it-> second.first;
        it-> second.first += 1;
        if (it-> second.first >= it-> second.second.size ()) {
          it-> second.first = 0;
        }

        LOG_INFO ("Find service : ", kind);
        auto src = it-> second.second [index];

        auto res = config::Dict ()
          .insert ("addr", std::make_shared <config::String> (src.addr))
          .insert ("id", std::make_shared <config::String> (src.id))
          .insert ("port", std::make_shared <config::Int> (src.port));

        return ResponseCode (200, res);
      }

      return ResponseCode (404);
    } catch (...) {}

    return ResponseCode (400);
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> RegistryService::closeService (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto kind = msg ["kind"].getStr ();
      auto id = msg ["id"].getStr ();
      auto addr = msg ["addr"].getStr ();
      uint32_t port = msg ["port"].getI ();

      LOG_INFO ("Closing service : ", kind, " ", id, " ", addr, " ", port);

      auto it = this-> _services.find (kind);
      if (it != this-> _services.end ()) {
        it-> second.first = 0;
        std::vector <Service> res;
        for (auto & jt : it-> second.second) {
          if (jt.addr != addr || jt.id != id || jt.port != port) {
            res.push_back (jt);
          }
        }

        if (res.size () == 0) {
          this-> _services.erase (kind);
        } else {
          it-> second.second = res;
        }
      }

      return ResponseCode (200);
    } catch (...) {}

    return ResponseCode (400);
  }


  std::shared_ptr <rd_utils::concurrency::actor::ActorRef> connectRegistry (rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode & conf) {
    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> act = nullptr;
    try {
      auto name = conf ["registry"].getOr ("name", "registry");
      auto addr = conf ["registry"].getOr ("addr", "127.0.0.1");
      uint32_t port = conf ["registry"].getOr ("port", 9090);

      act = sys-> remoteActor (name, net::SockAddrV4 (addr, port), false);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
    }

    if (act == nullptr) {
      throw std::runtime_error ("Failed to connect to registry");
    }

    return act;
  }

  void registerService (std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind, const std::string & name, uint32_t port, const std::string & iface) {
    std::string addr = rd_utils::net::Ipv4Address::getIfaceIp (iface).toString ();
    auto req = config::Dict ()
      .insert ("type", "register")
      .insert ("kind", kind)
      .insert ("id", name)
      .insert ("addr", addr)
      .insert ("port", (int64_t) port);

    LOG_INFO ("Registering to registry : ", kind, " ", name, " ", addr, " ", port);

    auto resp = registry-> request (req).wait ();
    if (resp == nullptr || resp-> getOr ("code", 0) != 200) {
      throw std::runtime_error ("Failed to register service : " + kind + "/" + name);
    }
  }

  std::shared_ptr <rd_utils::concurrency::actor::ActorRef> findService (rd_utils::concurrency::actor::ActorSystem * sys, std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind) {
    auto req = config::Dict ()
      .insert ("type", "get")
      .insert ("kind", kind);

    LOG_INFO ("Finding in registry : ", kind);

    auto resp = registry-> request (req).wait ();
    if (resp == nullptr || resp-> getOr ("code", 0) != 200) {
      throw std::runtime_error ("Failed to find service : " + kind + "/");
    }

    auto addr = (*resp) ["content"]["addr"].getStr ();
    auto id = (*resp) ["content"]["id"].getStr ();
    uint32_t port = (*resp) ["content"]["port"].getI ();

    auto act = sys-> remoteActor (id, net::SockAddrV4 (addr, port), false);
    if (act == nullptr) {
      throw std::runtime_error ("Failed to find service : " + kind + "/");
    }

    return act;
  }

  void closeService (std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind, const std::string & name, uint32_t port, const std::string & iface) {
    auto addr = rd_utils::net::Ipv4Address::getIfaceIp (iface).toString ();
    auto req = config::Dict ()
      .insert ("type", "close")
      .insert ("kind", kind)
      .insert ("id", name)
      .insert ("addr", addr)
      .insert ("port", (int64_t) port);

    LOG_INFO ("Closing to registry : ", kind, " ", name, " ", addr, " ", port);

    auto resp = registry-> request (req).wait ();
    if (resp == nullptr || resp-> getOr ("code", 0) != 200) {
      if (resp != nullptr) {
        std::cout << *resp << std::endl;
      }

      throw std::runtime_error ("Failed to unregister service : " + kind + "/" + name);
    }
  }


}
