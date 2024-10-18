#define LOG_LEVEL 10
#include "entity.hh"

#include "cachelib/allocator/memory/MemoryPool.h"
#include "cachelib/allocator/LruTailAgeStrategy.h"
#include "cachelib/common/Exceptions.h"
#include "cachelib/allocator/memory/Slab.h"

using namespace facebook;

namespace cachecache::instance {

  CacheEntity::CacheEntity () {}

  /**
   * ============================================================================================================
   * ============================================================================================================
   * =========================================      CONFIGURE     ===============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  void CacheEntity::configure (const std::string & name, size_t maxSize, uint64_t ttl) {
    this-> configureEntity (name, maxSize, ttl);
    this-> configureLru ();
    this-> configurePool ("default");
  }

  void CacheEntity::configureEntity (const std::string & name, size_t maxSize, uint64_t ttl) {
    try {
      auto config = cachelib::LruAllocator::Config ()
        .setCacheSize (maxSize)
        .setCacheName (name)
        .setAccessConfig ({25, 10})
        .validate ();

      this-> _entity = std::make_unique <cachelib::LruAllocator> (config);
      this-> _name = name;
      this-> _maxSize = maxSize;

      this-> _maxValueSize = (cachelib::Slab::kSize - sizeof (int)) - sizeof (char); // The first int is used to store the last time used info
      this-> _ttl = ttl;

    } catch (...) {
      LOG_ERROR ("Failed to configure cache");
      throw std::runtime_error ("Failed to configure cache");
    }
  }

  void CacheEntity::configureLru () {
    try {
      cachelib::LruTailAgeStrategy::Config ageConfig;
      ageConfig.slabProjectionLength = 0; // dont project or estimate tail age
      ageConfig.numSlabsFreeMem = 1;     // ok to have ~40 MB free memory in unused allocations
      this-> _entity-> startNewPoolResizer(std::chrono::milliseconds(500),
                                           99999,
                                           std::make_shared<cachelib::LruTailAgeStrategy> (ageConfig));
    } catch (...) {
      LOG_ERROR ("Failed to configure LRU strategy");
      throw std::runtime_error ("Failed to configure LRU strategy");
    }
  }

  void CacheEntity::configurePool (const std::string & name) {
    try {
      cachelib::LruAllocator::MMConfig memoryConfig;
      memoryConfig.lruRefreshTime = 0;
      memoryConfig.updateOnRead = true;
      memoryConfig.updateOnWrite = true;
      this-> _pool = this-> _entity-> addPool (name,
                                               this-> _entity-> getCacheMemoryStats ().ramCacheSize,
                                               {},
                                               memoryConfig);
    } catch (...) {
      LOG_ERROR ("Failed to configure pool");
      throw std::runtime_error ("Failed to configure pool");
    }
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * =========================================      RESIZE      =================================================
   * ============================================================================================================
   * ============================================================================================================
   */

  bool CacheEntity::resize (size_t newSize) {
    auto current = this-> _entity-> getPool (this-> _pool).getPoolSize ();

    // compute the new size to make it a power of kSize
    auto bound = std::max ((size_t) 1, newSize / facebook::cachelib::Slab::kSize) * facebook::cachelib::Slab::kSize;
    LOG_INFO ("Asking for a cache resize from ", current, " to ~", newSize, ". Will resize to ", bound);

    if (current == bound) return true;
    if (current > bound) { // Shrinking
      if (!this-> _entity-> shrinkPool (this-> _pool, current - bound)) return false;

      auto currMemUsage = this-> getCurrentMemoryUsage ();
      if (currMemUsage > bound) this-> reclaimSlabs (currMemUsage - bound);

      return true;
    }

    return this-> _entity-> growPool (this-> _pool, bound - current);
  }

  void CacheEntity::reclaimSlabs (size_t memSize) {
    auto nbSlabs = std::max ((size_t) 1, memSize / cachelib::Slab::kSize);
    LOG_INFO ("Reclaiming : ", memSize, " or ", nbSlabs, " slabs");

    this-> _entity-> updateNumSlabsToAdvise (nbSlabs);
    auto results = this-> _entity-> calcNumSlabsToAdviseReclaim ();

    if (results.advise) {
      for (auto & it : results.poolAdviseReclaimMap) {
        this-> reclaimSlabsInPool (it.first, it.second);
      }
    }
  }

  uint64_t CacheEntity::reclaimSlabsInPool (cachelib::PoolId poolId, uint64_t nbSlabs) {
    cachelib::LruTailAgeStrategy strategy;
    for (uint64_t reclaimed = 0 ; reclaimed < nbSlabs ; reclaimed ++) {
      const auto & classId = strategy.pickVictimForResizing (*this-> _entity, poolId);
      if (classId == cachelib::Slab::kInvalidClassId) {
        LOG_ERROR ("Invalid class id");
        return reclaimed;
      }

      try {
        this-> _entity-> releaseSlab (poolId, classId, cachelib::SlabReleaseMode::kAdvise);
      } catch (const cachelib::exception::SlabReleaseAborted & e) {
        LOG_ERROR ("Abort slab advising from ", (int) poolId, (int) classId, " because : ", e.what ());
        return reclaimed;
      } catch (const std::exception & e) {
        LOG_ERROR ("Error in slab advising from ", (int) poolId, (int) classId, " because : ", e.what (), ". Ignoring?");
      }
    }

    return nbSlabs;
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * ========================================      GETTERS      =================================================
   * ============================================================================================================
   * ============================================================================================================
   */

  size_t CacheEntity::getCurrentMemoryUsage () const {
    return this-> _entity-> getPool (this-> _pool).getCurrentAllocSize ();
  }

  size_t CacheEntity::getMaxSize () const {
    return this-> _maxSize;
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * ======================================      INSERT/FIND      ===============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  void CacheEntity::insert (const std::string & key, rd_utils::net::TcpSession & session) {
    auto valLen = session-> receiveI32 ();
    if (valLen > this-> _maxValueSize) {
      LOG_ERROR ("Value for key : ", key, " too large : ", valLen);
      session-> close ();
      return;
    }

    try {
      auto handle = this-> _entity-> allocate (this-> _pool, key.c_str (), valLen + sizeof (int) + sizeof (char), this-> _ttl);
      if (!handle) {
        LOG_ERROR ("Cannot allocate, cache is full");
        session-> sendI32 (0);
        return;
      }

      *static_cast <int*> (handle-> getMemory ()) = time (NULL);
      if (this-> receiveValue (static_cast <char*> (handle-> getMemory ()) + sizeof (int), valLen, session)) {
        this-> _entity-> insertOrReplace (handle);
      }

      session-> sendI32 (1);
    } catch (const std::exception & e) {
      LOG_ERROR ("Failed to insert key : ", key, " because ", e.what ());
      session-> sendI32 (0);
    }
  }

  void CacheEntity::find (const std::string & key, rd_utils::net::TcpSession & session) {
    try {
      auto handle = this-> _entity-> findToWrite (key.c_str ());
      if (handle != nullptr) {
        size_t valLen = handle-> getSize () - sizeof (int);
        session-> sendI32 (valLen);
        session-> send (static_cast <const char*> (handle-> getMemory ()) + sizeof (int), valLen);

        *static_cast <int*> (handle-> getMemory ()) = time (NULL);
      } else {
        session-> sendI32 (0);
      }
    } catch (std::exception & e) {
      LOG_ERROR ("Failed to find key : ", key, " because ", e.what ());
      session-> sendI32 (0);
    }
  }

  bool CacheEntity::receiveValue (char * memory, uint64_t len, rd_utils::net::TcpSession & session) {
    auto rcv = session-> receive (memory, len);

    if (rcv == len) {
      memory [rcv] = 0;
      return true;
    } else {
      session-> close ();
      return false;
    }
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * ========================================      DISPOSE      =================================================
   * ============================================================================================================
   * ============================================================================================================
   */

  void CacheEntity::dispose () {
    this-> _entity.reset ();
    this-> _entity = nullptr;
    this-> _pool = 0;
    this-> _maxValueSize = 0;
    this-> _maxSize = 0;
    this-> _name = "";
  }

  CacheEntity::~CacheEntity () {
    this-> dispose ();
  }

}
