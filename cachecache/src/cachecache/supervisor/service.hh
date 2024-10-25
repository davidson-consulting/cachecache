#pragma once

#include <map>
#include <rd_utils/_.hh>
#include <cachecache/utils/codes/_.hh>

#include "market.hh"

namespace cachecache::supervisor {

  /**
   * Supervisor service actor
   * The supervisor is managing a pool of cache entities and communicate with them using the actor system
   */
  class SupervisorService : public rd_utils::concurrency::actor::ActorBase {
  private:

    struct CacheInfo {
      // The actor managing the cache instance
      std::shared_ptr <rd_utils::concurrency::actor::ActorRef> remote;

      // The name of the cache (for debug purpose)
      std::string name;

      // The uniq id of the cache
      uint64_t uid;
    };

  private:

    // The cache instances
    std::map <uint32_t,  CacheInfo> _instances;

    // counter to create UIDs
    uint32_t _lastUid = 1;

    // The market routine thread
    rd_utils::concurrency::Thread _marketRoutine;

    // The frequency of the market
    float _freq;

    // The sleep or wait in market routine
    rd_utils::concurrency::semaphore _marketSleeping;

    // The size of the memory managed by the service in KB
    rd_utils::utils::MemorySize _memoryPoolSize;

    // The configuration of the supervisor
    std::shared_ptr <rd_utils::utils::config::ConfigNode> _cfg;

    // while true the market routine runs
    bool _running;

    // Mutex of the actor
    rd_utils::concurrency::mutex _m;

    // The market of the supervisor, used to select the size of the cache entities
    Market _market;

  private :

    static std::shared_ptr <rd_utils::concurrency::actor::ActorSystem>  __GLOBAL_SYSTEM__;

  public:

    /**
     * @params:
     *    - name: the uniq name of the service
     *    - sys: the actor system managing the service
     *    - cfg: the configuration of the supervisor
     */
    SupervisorService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg);

    /**
     * Triggered when the actor is registered
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

  private:

    /**
     * Configure the service
     */
    void configure (std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg);

    /**
     * Register a new cache entity
     * @params:
     *    - cfg: the message sent by the entity
     */
    std::shared_ptr <rd_utils::utils::config::ConfigNode> registerCache (const rd_utils::utils::config::ConfigNode & cfg);

    /**
     * Erase a cache instance
     */
    void eraseCache (const rd_utils::utils::config::ConfigNode & cfg);

    /**
     * Dispose the service, and kill all attached cache entities
     */
    void dispose ();


  private :

    /**
     *  The market routine defining the amount of memory to give to each cache entities
     */
    void marketRoutine (rd_utils::concurrency::Thread);

    /**
     * Update the entity usage informations by requesting it to the cache services
     * @assume: thread mutex lock
     */
    void updateEntityInfo ();

    /**
     * Sent the decision of the market to the cache instances
     * @assume: thread mutex lock
     */
    void informEntitySizes ();

  /**
   * ============================================================================================================
   * ============================================================================================================
   * =========================================      STATIC      =================================================
   * ============================================================================================================
   * ============================================================================================================
   */

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
