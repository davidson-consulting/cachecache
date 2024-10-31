#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>

namespace socialNet {

  struct Service {
    std::string id;
    std::string addr; // add of the system
    uint32_t port; // port of the system
  };

  class RegistryService :  public rd_utils::concurrency::actor::ActorBase {

    std::map <std::string, std::pair <uint32_t, std::vector <Service> > > _services;

  public:

    RegistryService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys);
    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onQuit () override;

  private:

    std::shared_ptr<rd_utils::utils::config::ConfigNode> registerService (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr<rd_utils::utils::config::ConfigNode> findService (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr<rd_utils::utils::config::ConfigNode> closeService (const rd_utils::utils::config::ConfigNode & msg);

  };

  std::shared_ptr <rd_utils::concurrency::actor::ActorRef> connectRegistry (rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode & conf);
  void registerService (std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind, const std::string & name, uint32_t port, const std::string & iface);
  std::shared_ptr <rd_utils::concurrency::actor::ActorRef> findService (rd_utils::concurrency::actor::ActorSystem * sys, std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind);
  void closeService (std::shared_ptr <rd_utils::concurrency::actor::ActorRef> registry, const std::string & kind, const std::string & name, uint32_t port, const std::string & iface);
}
