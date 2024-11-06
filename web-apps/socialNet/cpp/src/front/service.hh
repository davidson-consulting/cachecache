#pragma once

#include <httpserver.hpp>
#include <rd_utils/_.hh>
#include "login.hh"
#include "newuser.hh"
#include "submit.hh"
#include "home.hh"
#include "user.hh"
#include "subs.hh"
#include "follow.hh"

#include <utils/cache/client.hh>

namespace socialNet {

        class FrontServer : public httpserver::http_resource {
        private:

                // The web server used to answer user requests
                std::shared_ptr <httpserver::webserver> _server;

                // The login resource
                LoginRoute _login;

                // The logon resource
                NewUserRoute _logon;

                // The submit resource
                SubmitNewPostRoute _submit;

                // The resource to get the length of the timeline
                HomeTimelineLenRoute _homeTimelineLen;

                // The resource to get the length of the timeline
                HomeTimelineRoute _homeTimeline;

                // The resource to get the length of the timeline of user posts
                UserTimelineLenRoute _userTimelineLen;

                // The resource to get the length of the timeline of user posts
                UserTimelineRoute _userTimeline;

                // Resource to get the number of subscirptions of a user
                SubscriptionLenRoute _subLen;

                // Resource to get the subscirptions of a user
                SubscriptionRoute _subs;

                // Resource to get the number of followers of a user
                FollowerLenRoute _followerLen;

                // Resource to get the followers of a user
                FollowerRoute _followers;

                // Resource to add a follower to a user
                FollowRoute _newFollow;

                // Resource to rm a follower to a user
                UnfollowRoute _rmFollow;

                // Resource to check if a user is following another user
                IsFollowingRoute _isFollow;

                // The actor system used by the front to communicate with registry
                std::shared_ptr <rd_utils::concurrency::actor::ActorSystem> _sys;

                // The connection to the registry
                std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;

                // The connection to the cache
                std::shared_ptr <socialNet::utils::CacheClient> _cache;

        public:

                FrontServer ();

                /**
                 * Configure the server
                 */
                void configure (const rd_utils::utils::config::ConfigNode & cfg);

                /**
                 * Start the web server
                 */
                void start ();

                /**
                 * Dispose the web server resources
                 */
                void dispose ();

                /**
                 * @returns: the actor sytem used by the front end
                 */
                rd_utils::concurrency::actor::ActorSystem* getSystem ();

                /**
                 * @returns: the actor reference to the registry
                 */
                std::shared_ptr <rd_utils::concurrency::actor::ActorRef> getRegistry ();

                /**
                 * @returns: the cache of the service
                 */
                std::shared_ptr <socialNet::utils::CacheClient> getCache ();

                /**
                 * this-> dispose ();
                 */
                ~FrontServer ();

        };

}
