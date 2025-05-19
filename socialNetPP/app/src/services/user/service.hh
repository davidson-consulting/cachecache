#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>

namespace socialNet::user {

  class UserDatabase;
  class UserService : public rd_utils::concurrency::actor::ActorBase {
  private:

    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;
    bool _needClose = true;

    rd_utils::utils::config::Dict _conf;
    uint32_t _uid = 0;
    std::string _iface;

    std::shared_ptr <UserDatabase> _db;

  public:

    /**
     * @params:
     *    - name: the name of the actor
     *    - sys: the system responsible of the actor
     *    - configFile: the configuration file used to configure db access and other stuff
     */
    UserService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & configFile);

    void onStart () override;

    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onMessage (const rd_utils::utils::config::ConfigNode & msg);

    void onQuit () override;

  private:

    std::shared_ptr<rd_utils::utils::config::ConfigNode> registerUser (const rd_utils::utils::config::ConfigNode & node);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> login (const rd_utils::utils::config::ConfigNode & node);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> find (const rd_utils::utils::config::ConfigNode & node);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> findByLoginMultiple (const rd_utils::utils::config::ConfigNode & node);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> findByIdMultiple (const rd_utils::utils::config::ConfigNode & node);
  };

}
