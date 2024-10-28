#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include "database.hh"

namespace socialNet::post {

  class PostStorageService : public rd_utils::concurrency::actor::ActorBase {
  private:

    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;

    std::string _iface;

    PostDatabase _db;

  public:

    /**
     * @params:
     *    - name: the name of the actor
     *    - sys: the system responsible of the actor
     *    - configFile: the configuration file used to configure db access and other stuff
     */
    PostStorageService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf);

    /**
     * Request for insertions
     */
    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Request for data retreival
     */
    std::shared_ptr<rd_utils::memory::cache::collection::ArrayListBase> onRequestList (const rd_utils::utils::config::ConfigNode & msg);

    void onStream (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream);

    void onQuit () override;

  private :

    /**
     * Store a new post
     */
    std::shared_ptr <rd_utils::utils::config::ConfigNode> store (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Read one post
     */
    std::shared_ptr <rd_utils::utils::config::ConfigNode> readOne (const rd_utils::utils::config::ConfigNode & msg);


    void streamPosts (rd_utils::concurrency::actor::ActorStream & stream);

  };



}
