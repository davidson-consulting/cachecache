#pragma once

#include <cstdint>
#include <map>
#include <rd_utils/utils/mem_size.hh>


namespace cachecache::supervisor {

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

      // The current usage of the cache in KB
      rd_utils::utils::MemorySize usage;

      // Storing the last size to avoid resizing what does not need to be resized
      rd_utils::utils::MemorySize last;

      // The wallet of the cache
      uint64_t wallet;

    };

  private:

    // The entities managed by the market
    std::map <uint64_t, CacheStatus> _entities;

    // The size in KB of the memory that can be distributed among the caches
    rd_utils::utils::MemorySize _size;

  public:

    /**
     * @params:
     *    - size: the size in KB that can be distributed among the caches
     */
    Market (rd_utils::utils::MemorySize size);

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
     * @returns: the size attributed by the market to the cache entity
     */
    rd_utils::utils::MemorySize getCacheSize (uint64_t uid) const;

    /**
     * @returns: true if the market made the size of the cache change
     */
    bool hasChanged (uint64_t uid) const;

    /**
     *
     */
    ~Market ();

  };


}
