#pragma once

#include <string>
#include <cstdint>

namespace cachecache::client {

  /**
   * A cache entity is a cachelib instance used as a key/value store
   */
  class CacheEntity {
  private:

    // The uniq name of the entity
    std::string _name;

    // The maximum size that can be taken by the cache
    size_t _maxSize;

    // The requested size for the cache instance
    size_t _requested;

    // The cache managed by the entity
    std::unique_ptr <facebook::cachelib::LruAllocator> _cacheEntity;

  public:

    /**
     * ========================================================
     * ========================================================
     * ====================   DELETED  ========================
     * ========================================================
     * ========================================================
     */
    CacheEntity (const CacheEntity &) = delete;
    void operator=(const CacheEntity&) = delete;
    CacheEntity (CacheEntity &&) = delete;
    void operator=(CacheEntity &&) = delete;

  public:

    CacheEntity ();

    /**
     * Configure the cache
     * @params:
     *    - name: the uniq name of the cache
     *    - maxSize: the maximum size the cache can take
     *    - requested: ?
     */
    void configure (const std::string & name, size_t maxSize, size_t requested);

    /**
     * Resize the cache entity to take a new memory space
     * @params:
     *    - newSize: the new size taken by the cache in bytes
     * @assume: 'newSize' < this-> getMaxSize ()
     */
    bool resize (size_t newSize);

    /**
     * @returns: the maximum size that can be taken by the cache
     */
    size_t getMaxSize () const;

    /**
     * @returns: the memory actually used by the cache
     */
    size_t getCurrentMemoryUsage () const;

    /**
     * Dispose all content of the cache, and free all handles
     */
    void dispose ();

    /**
     * @cf: this-> dispose ();
     */
    ~CacheEntity ();

  private:

    /**
     * Shrink to cache to target a new size
     */
    void shrink (size_t newSize);

  };


}
