#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>
#include "database.hh"

#include <set>

namespace socialNet::timeline {

  class TimelineService : public rd_utils::concurrency::actor::ActorBase {
  private:

    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;

    std::string _iface;

    TimelineDatabase _db;

    std::string _issuer;
    std::string _secret;

  public:

    /**
     * @params:
     *    - name: the name of the actor
     *    - sys: the system responsible of the actor
     *    - configFile: the configuration file used to configure db access and other stuff
     */
    TimelineService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onStream (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream);

    void onMessage (const rd_utils::utils::config::ConfigNode & msg);

    void onQuit () override;

  private:

    std::shared_ptr<rd_utils::utils::config::ConfigNode> updatePosts (const rd_utils::utils::config::ConfigNode & msg);
    void updateForFollowers (uint32_t uid, uint32_t postId, const std::set <int64_t> & tagged);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> countPosts (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr<rd_utils::utils::config::ConfigNode> countHome (const rd_utils::utils::config::ConfigNode & msg);

    void streamHome (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream);
    void streamPosts (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream);

  };

}
