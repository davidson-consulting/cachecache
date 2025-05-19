#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>
#include "database.hh"

namespace socialNet::post {

  class PostStorageService : public rd_utils::concurrency::actor::ActorBase {
  private:

    uint32_t _uid;
    rd_utils::utils::config::Dict _conf;
    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;
    bool _needClose = true;

    std::string _iface;
    std::shared_ptr <PostDatabase> _db;

    std::string _issuer;
    std::string _secret;

  public:

    /**
     * @params:
     *    - name: the name of the actor
     *    - sys: the system responsible of the actor
     *    - configFile: the configuration file used to configure db access and other stuff
     */
    PostStorageService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf);

    void onStart () override;

    /**
     * Request for insertions
     */
    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onMessage (const rd_utils::utils::config::ConfigNode & msg);

    void onQuit () override;

  private :

    std::shared_ptr <rd_utils::utils::config::ConfigNode> store (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr <rd_utils::utils::config::ConfigNode> readOne (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr <rd_utils::utils::config::ConfigNode> readMultiple (const rd_utils::utils::config::ConfigNode & msg);


  };



}
