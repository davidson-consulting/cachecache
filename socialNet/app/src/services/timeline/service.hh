#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>
// #include "database.hh"
#include "file_base.hh"

#include <set>

namespace socialNet::timeline {

        class TimelineService : public rd_utils::concurrency::actor::ActorBase {
        private:

                struct PostUpdate {
                        uint32_t pid;
                        std::set <uint32_t> tagged;
                };

                std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;
                bool _needClose = true;
                // std::shared_ptr <utils::MysqlClient::Statement> _req100;
                // std::shared_ptr <utils::MysqlClient::Statement> _req10;

                rd_utils::concurrency::mutex _m;
                rd_utils::concurrency::Thread _routine;
                rd_utils::concurrency::semaphore _routineSleeping;

                bool _running;
                float _freq;

                std::map <uint32_t, std::vector <PostUpdate> > _toUpdates;
                std::string _iface;

                // TimelineDatabase _insertDb;
                // TimelineDatabase _readDb;

                TimelineFileBase _db;

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

                void onStart () override;

                std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

                void onMessage (const rd_utils::utils::config::ConfigNode & msg);

                void onQuit () override;

        private:

                void updatePosts (const rd_utils::utils::config::ConfigNode & msg);

                std::shared_ptr<rd_utils::utils::config::ConfigNode> countPosts (const rd_utils::utils::config::ConfigNode & msg);
                std::shared_ptr<rd_utils::utils::config::ConfigNode> countHome (const rd_utils::utils::config::ConfigNode & msg);

                std::shared_ptr<rd_utils::utils::config::ConfigNode> readHome (const rd_utils::utils::config::ConfigNode & msg);
                std::shared_ptr<rd_utils::utils::config::ConfigNode> readPost (const rd_utils::utils::config::ConfigNode & msg);


                void treatRoutine (rd_utils::concurrency::Thread);
                void updateForFollowers (uint32_t uid, const std::vector <PostUpdate> & up);

        };

}
