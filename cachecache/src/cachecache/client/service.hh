#pragma once

#include <rd_utils/_.hh>
#include <cachecache/utils/codes/_.hh>

namespace cachecache::client {

  /**
   * Actor that communicates with the supervisor
   */
  class CacheService : public rd_utils::concurrency::actor::ActorBase {
  private:

    // The service network interface
    std::string _iface;

    // The reference to the supervisor actor
    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _supervisor;

    // The configuration of the actor
    std::shared_ptr <rd_utils::utils::config::ConfigNode> _cfg;

    // The actor system managing the actor
    static std::shared_ptr <rd_utils::concurrency::actor::ActorSystem>  __GLOBAL_SYSTEM__;

    // The cache entity managed by the actor
    // CacheEntity _entity;

  public:

    /**
     * Spawn a new Cache Service managing a cache entity
     * @params:
     *    - name: the uniq name of the entity
     *    - sys: the actor system of
     *    - cfg: the configuration of the cache
     */
    CacheService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg);

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

    static void deploy (int argc, char ** argv);
    static std::string appOption (int argc, char ** argv);

  private:

    /**
     * Connect the cache service actor to the supervisor
     * @params:
     *    - cfg: the configuration passed to the service
     */
    void connectSupervisor (const std::shared_ptr <rd_utils::utils::config::ConfigNode> conf);

  };

}
