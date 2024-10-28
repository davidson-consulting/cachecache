#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include "database.hh"

namespace socialNet::user {

  class UserService : public rd_utils::concurrency::actor::ActorBase {
  private:

    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;

    std::string _iface;

    UserDatabase _db;

  public:

    /**
     * @params:
     *    - name: the name of the actor
     *    - sys: the system responsible of the actor
     *    - configFile: the configuration file used to configure db access and other stuff
     */
    UserService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & configFile);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onStream (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream) override;

    void onQuit () override;

  private:

    void streamFindByLogin (rd_utils::concurrency::actor::ActorStream &);
    void streamFindById (rd_utils::concurrency::actor::ActorStream &);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> registerUser (const rd_utils::utils::config::ConfigNode & node);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> login (const rd_utils::utils::config::ConfigNode & node);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> find (const rd_utils::utils::config::ConfigNode & node);
  };

}
