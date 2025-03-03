#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>
#include "database.hh"

namespace socialNet::social_graph {

    class SocialGraphService : public rd_utils::concurrency::actor::ActorBase {
    private:

        std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;
        bool _needClose = true;

        rd_utils::utils::config::Dict _conf;
        uint32_t _uid;
        std::string _iface;

        std::shared_ptr <SocialGraphDatabase> _db;

        std::string _issuer;
        std::string _secret;

    public:

        /**
         * @params:
         *    - name: the name of the actor
         *    - sys: the system responsible of the actor
         *    - configFile: the configuration file used to configure db access and other stuff
         */
        SocialGraphService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf);

        void onStart () override;

        std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

        void onMessage (const rd_utils::utils::config::ConfigNode& msg);

        void onQuit () override;

    private :

        std::shared_ptr <rd_utils::utils::config::ConfigNode> subscribe (const rd_utils::utils::config::ConfigNode & msg);

        std::shared_ptr <rd_utils::utils::config::ConfigNode> deleteSub (const rd_utils::utils::config::ConfigNode & msg);

        std::shared_ptr <rd_utils::utils::config::ConfigNode> isFollowing (const rd_utils::utils::config::ConfigNode & msg);

        std::shared_ptr <rd_utils::utils::config::ConfigNode> countSubs (const rd_utils::utils::config::ConfigNode & msg);

        std::shared_ptr <rd_utils::utils::config::ConfigNode> countFollows (const rd_utils::utils::config::ConfigNode & msg);

        std::shared_ptr <rd_utils::utils::config::ConfigNode> readFollowers (const rd_utils::utils::config::ConfigNode & msg);

        std::shared_ptr <rd_utils::utils::config::ConfigNode> readSubscriptions (const rd_utils::utils::config::ConfigNode & msg);

    };

}
