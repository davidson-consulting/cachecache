#pragma once

#include <rd_utils/_.hh>
#include <cachecache/utils/codes/_.hh>
#include "entity.hh"

namespace cachecache::instance {

        /**
         * Actor that communicates with the supervisor
         */
        class CacheService : public rd_utils::concurrency::actor::ActorBase {
        private:

                // The service network interface
                std::string _ifaceAddr;

                // The uniq id according to the supervisor
                uint64_t _uid;

                // The memory pool size according to the supervisor
                rd_utils::utils::MemorySize _regSize;

                // The reference to the supervisor actor
                std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _supervisor;

                // The configuration of the actor
                std::shared_ptr <rd_utils::utils::config::ConfigNode> _cfg;

                // The tcp server used to communicate with the clients of the cache entity
                std::shared_ptr <rd_utils::net::TcpServer> _clients;

                // The actor system managing the actor
                static std::shared_ptr <rd_utils::concurrency::actor::ActorSystem>  __GLOBAL_SYSTEM__;

                // The cache entity managed by the actor
                std::shared_ptr <CacheEntity> _entity;

                rd_utils::concurrency::mutex _m;

                // Set to true when everything is set and running
                bool _fullyConfigured = false;

                // THe number of the instance in local system
                uint32_t _uniqNb;

        public:

                /**
                 * Spawn a new Cache Service managing a cache entity
                 * @params:
                 *    - name: the uniq name of the entity
                 *    - sys: the actor system of
                 *    - cfg: the configuration of the cache
                 */
                CacheService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg, uint32_t uniqNb);

                /**
                 * Triggered when the actor is register
                 */
                void onStart () override;

                /**
                 * Triggered when a message that does not require an answer is sent to the actor
                 * @params:
                 *    - msg: the message sent to the actor
                 */
                void onMessage (const rd_utils::utils::config::ConfigNode & msg) override;

                /**
                 * Triggered when a request is sent from another actor
                 * @params:
                 *    - msg: the message sent to the actor
                 * @returns: the response to that message
                 */
                std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

                /**
                 * Triggered when the actor is killed
                 */
                void onQuit () override;

        public:

                /**
                 * Answer a client request
                 */
                void onClient (std::shared_ptr<rd_utils::net::TcpStream> session);

                /**
                 * Client request a set
                 * @params:
                 *    - session: the session with a client
                 */
                void onSet (rd_utils::net::TcpStream& session);

                /**
                 * Client request a get
                 * @params:
                 *    - session: the session with a client
                 */
                void onGet (rd_utils::net::TcpStream& session);

                /**
                 * Read a string in the session whose size is at most 'size'
                 * @params:
                 *    - session: the opened session to the client
                 *    - size: the size to read
                 */
                std::string readStr (rd_utils::net::TcpStream& session, uint32_t size);

        public:

                /**
                 * Deploy the cache service and all attached elements
                 */
                static void deploy (int argc, char ** argv);

                /**
                 * Parse app option
                 */
                static std::string appOption (int argc, char ** argv);

                /**
                 * Function called when app is killed
                 */
                static void ctrlCHandler (int signum);

        private:

                /**
                 * Connect the cache service actor to the supervisor
                 * @params:
                 *    - conf: the configuration passed to the service
                 */
                bool connectSupervisor (const std::shared_ptr <rd_utils::utils::config::ConfigNode> conf);

                /**
                 * Configure the cache entity
                 * @params:
                 *    - conf: the configure passed to the service
                 */
                void configureEntity (const std::shared_ptr <rd_utils::utils::config::ConfigNode> conf);

                /**
                 * Configure the client side server
                 * @params:
                 *    - cfg: the configuration passed to the service
                 */
                void configureServer (const std::shared_ptr <rd_utils::utils::config::ConfigNode> conf);

                /**
                 * Apply a resize of the cache entity
                 */
                void onSizeUpdate (const rd_utils::utils::config::ConfigNode & msg);

                /**
                 * Answer to a supervisor request for the current cache usage
                 */
                std::shared_ptr <rd_utils::utils::config::ConfigNode> onEntityInfoRequest (const rd_utils::utils::config::ConfigNode & msg);

        };

}
