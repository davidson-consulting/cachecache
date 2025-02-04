#pragma once

#include <cstdint>
#include <map>
#include <rd_utils/utils/mem_size.hh>
#include "history.hh"


namespace kv_store::supervisor {

  /**
   * A market that sells memory spaces to caches
   */
  class Market {

    /**
     * Storing cache information
     */
    struct CacheStatus {

      // The uniq id of the cache
      uint64_t uid;

      // The size requested by the cache in KB
      rd_utils::utils::MemorySize req;

      // The size given by the market to the cache in KB
      rd_utils::utils::MemorySize size;

      // The size given by the market during paid auction phase KB
      rd_utils::utils::MemorySize buyingSize;

      // Last given size
      rd_utils::utils::MemorySize last;

      // History of cache memory usage
      History usages;

      // The wallet of the cache
      rd_utils::utils::MemorySize wallet;

    };

    /**
     * Storing Market configuration
     */
    struct MarketConfig {
      rd_utils::utils::MemorySize size;
      float triggerIncrement;
      float increasingSpeed;
      float triggerDecrement;
      float decreasingSpeed;
    };

  private:

    // The entities managed by the market
    std::map <uint64_t, CacheStatus> _entities;

    // The size in KB of the memory that can be distributed among the caches
    rd_utils::utils::MemorySize _size;

    // The percentage of usage of memory before an increment of memory
    // allocation
    float _triggerIncrement;

    // The percentage of usage of memory before a decrement of memory 
    // allocation
    float _triggerDecrement;

    // The speed of the increment of memory
    float _increasingSpeed;

    // The percentage of usage in memory in VM before a decrement
    // of memory allocation
    float _decreasingSpeed;

  public:

    /**
     * Init the cache with default parameters
     * @params:
     *    - size: the size in KB that can be distributed among the caches
     */
    Market (rd_utils::utils::MemorySize size);

    /**
     * Init the cache with a provided configuration
     * @params:
     *  - cfg: The MarketConfig configuration
     */
    Market (const MarketConfig& cfg);

    /**
     * Register a new cache in the market
     * @params:
     *    - uid: the uniq id of the cache
     *    - req: the request size of the cache
     *    - usage: the actual usage of the cache
     */
    void registerCache (uint64_t uid, rd_utils::utils::MemorySize req, rd_utils::utils::MemorySize usage);

    /**
     * Remove a cache instance
     * @params:
     *    - uid: the uid of the cache to remove
     */
    void removeCache (uint64_t uid);

    /**
     * Run a market iteration
     * @info: affect a size to every cache instances
     */
    void run ();

    /**
     * Update the usage of a cache entity
     * @params:
     *    - uid: the uid of the cache instance
     *    - usage: the usage in KB of the instance
     */
    void updateUsage (uint64_t uid, rd_utils::utils::MemorySize usage);

    /**
     * Update the size of a cache entity
     * Used to synchronize the market information with 
     * the actual size of the remote cache (can differ from
     * what the market advice when running)
     * @params:
     *    - uid: the uid of a cache entity
     *    - size: the size of the instance
     */
    void updateSize (uint64_t uind, rd_utils::utils::MemorySize size);

    /**
     * @returns: the size attributed by the market to the cache entity
     */
    rd_utils::utils::MemorySize getCacheSize (uint64_t uid) const;

    /**
     * @returns: the size of the market to distribute among the cache instances
     */
    rd_utils::utils::MemorySize getPoolSize () const;

    /**
     * @returns: true if the market made the size of the cache change
     */
    bool hasChanged (uint64_t uid) const;

    /**
     *
     */
    ~Market ();

  private:
    /**
     * Sell base memory among the caches. It aims to ensure that 
     * each cache can access enough memory for their usage in 
     * the limit of their requested memory. Extra (> requested
     * memory) can be allocated in the buyExtraMemory function.
     * @params:
     *  - market: total amount of available memory. Will be updated
     *  to be equal to remaining, available, memory after base memory
     *  is sold.
     *  - buyers: how much memory each cache wants to buy?
     * @return:
     *  - A mapping between each cache id and its memory allocation
     */
    std::map<uint64_t, rd_utils::utils::MemorySize> sellBaseMemory(rd_utils::utils::MemorySize& market, std::map<uint64_t, rd_utils::utils::MemorySize>& buyers);

    /**
     * Sell extra memory for all caches that needs more memory 
     * than their requested amount. If a cache fails to buy 
     * its needed memory, the amount it needs is updated into 
     * the buyer parameter
     * @params:
     * - allocated: amount of memory allocated to each cache. Will be 
     *   updated if cache needs and can afford more
     *  - buyers: amount of memory each cache needs to buy. Will try 
     *  to buy the extra needed memory. Will be reduced to 0 if can 
     *  afford all or reduced to the amount the cache can't buy.
     *  - market: the amount of remaining available memory in the market
     *  - allNeeded: the total amount of extra needed memory caches 
     *  could not buy during the function execution
     */
    void buyExtraMemory(std::map<uint64_t, rd_utils::utils::MemorySize> & allocated,
                        std::map<uint64_t, rd_utils::utils::MemorySize> & buyers,
                        rd_utils::utils::MemorySize & market,
                        rd_utils::utils::MemorySize & allNeeded);

    /**
     * Increase a the wallet of the desired cache by a certain amount 
     * while ensuring it does not reach the max wallet size
     * @params:
     *  - cacheId: the identifier of the cache whose wallet will be 
     *  updated
     *  - amount: the amount of memory to try to add to the wallet 
     *  (the function ensure that cacheId's wallet wont go over the 
     *  configured limit)
     */
    void increaseWallet(uint64_t cacheId, rd_utils::utils::MemorySize amount);
  };
}
