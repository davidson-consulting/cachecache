#pragma once

#include <map>
#include <rd_utils/_.hh>
#include <cachecache/utils/codes/_.hh>

namespace cachecache::supervisor {

  /**
   * Supervisor service actor
   * The supervisor is managing a pool of cache entities and communicate with them using the actor system
   */
  class SupervisorService : public rd_utils::concurrency::actor::ActorBase {
  private:

    struct CacheInfo {
      std::shared_ptr <rd_utils::concurrency::actor::ActorRef> remote;
    };

  private:

    // The cache instances
    std::map <std::string,  CacheInfo> _instances;

    // counter to create UIDs
    uint32_t _lastUid = 1;

    static std::shared_ptr <rd_utils::concurrency::actor::ActorSystem>  __GLOBAL_SYSTEM__;

  public:

    /**
     * @params:
     *    - name: the uniq name of the service
     *    - sys: the actor system managing the service
     *    - cfg: the configuration of the supervisor
     */
    SupervisorService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode & cfg);

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

  private:

    /**
     * Register a new cache entity
     * @params:
     *    - cfg: the message sent by the entity
     */
    std::shared_ptr <rd_utils::utils::config::ConfigNode> registerCache (const rd_utils::utils::config::ConfigNode & cfg);

    /**
     * Dispose the service, and kill all attached cache entities
     */
    void dispose ();

  public:

    /**
     * Deploy the service and all attached cache instances
     */
    static void deploy (int argc, char ** argv);

  private:

    /**
     * Parse app options
     */
    static std::string appOption (int argc, char ** argv);

    /**
     * Function called when app is killed
     */
    static void ctrlCHandler (int signum);

  };


}
