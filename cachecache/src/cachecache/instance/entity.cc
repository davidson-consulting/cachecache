#define LOG_LEVEL 10
#include "entity.hh"

#include "cachelib/allocator/memory/MemoryPool.h"
#include "cachelib/allocator/LruTailAgeStrategy.h"
#include "cachelib/common/Exceptions.h"
#include "cachelib/allocator/memory/Slab.h"

using namespace facebook;
using namespace rd_utils::utils;

namespace cachecache::instance {

  CacheEntity::CacheEntity () :
    _maxSize (MemorySize::B (0))
    , _maxValueSize (MemorySize::B (0))
    , _requested (MemorySize::B (0))
    , _entity (nullptr)
  {}

  /**
   * ============================================================================================================
   * ============================================================================================================
   * =========================================      CONFIGURE     ===============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  void CacheEntity::configure (const std::string & name, rd_utils::utils::MemorySize maxSize, uint32_t ttl) {
    this-> configureEntity (name, maxSize, ttl);
    this-> configureLru ();
    this-> configurePool ("default");
  }

  void CacheEntity::configureEntity (const std::string & name, rd_utils::utils::MemorySize maxSize, uint32_t ttl) {
    try {
      auto config = cachelib::LruAllocator::Config ()
        .setCacheSize (maxSize.bytes ())
        .setCacheName (name)
        .setAccessConfig ({25, 10})
        .validate ();

      this-> _entity = std::make_unique <cachelib::LruAllocator> (config);
      this-> _name = name;
      this-> _maxSize = maxSize;

      this-> _maxValueSize = CacheEntity::getSlabSize () - MemorySize::B (sizeof (int)) - MemorySize::B (sizeof (char)); // The first int is used to store the last time used info
      this-> _ttl = ttl;

      LOG_INFO ("Cache configured  maxSize=", this-> _maxSize.megabytes (), "MB, maxValueSize=", this-> _maxValueSize.megabytes (), "MB, ttl=", this-> _ttl, "s");
    } catch (...) {
      LOG_ERROR ("Failed to configure cache");
      throw std::runtime_error ("Failed to configure cache");
    }
  }

  void CacheEntity::configureLru () {
    try {
      cachelib::LruTailAgeStrategy::Config ageConfig;
      ageConfig.slabProjectionLength = 0; // dont project or estimate tail age
      ageConfig.numSlabsFreeMem = 100;     // ok to have ~40 MB free memory in unused allocations
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

  bool CacheEntity::resize (rd_utils::utils::MemorySize newSize) {
    if (this-> _entity == nullptr) return false;

    auto current = MemorySize::B (this-> _entity-> getPool (this-> _pool).getPoolSize ());

    auto asked = MemorySize::roundUp (newSize, CacheEntity::getSlabSize ());
    auto bound = MemorySize::min (MemorySize::max (CacheEntity::getSlabSize (), asked), this-> _maxSize);

    LOG_INFO ("Asking for a cache resize from ", current.megabytes (), "MB to ~", newSize.megabytes (), " as ", asked.megabytes (), " MB. Will resize to ", bound.megabytes (), "MB");

    if (current.bytes () < bound.bytes ()) { // Growing
      return this-> _entity-> growPool (this-> _pool, (bound - current).bytes ());
    }

    else if (current.bytes () > bound.bytes ()) { // Shrinking
      if (!this-> _entity-> shrinkPool (this-> _pool, (current - bound).bytes ())) return false;

      // auto currMemUsage = this-> getCurrentMemoryUsage ();
      // if (currMemUsage.bytes () > bound.bytes ()) this-> reclaimSlabs (currMemUsage - bound);

      return true;
    }

    return true;
  }

  void CacheEntity::reclaimSlabs (rd_utils::utils::MemorySize memSize) {
    auto reclaim = MemorySize::max (CacheEntity::getSlabSize (), memSize);
    uint64_t nbSlabs = reclaim.bytes () / CacheEntity::getSlabSize ().bytes ();
    LOG_INFO ("Reclaiming : ", reclaim.megabytes (), "MB or ", nbSlabs, " slabs");

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

  MemorySize CacheEntity::getCurrentMemoryUsage () const {
    if (this-> _entity == nullptr) return MemorySize::B (0);
    return MemorySize::B (this-> _entity-> getPool (this-> _pool).getCurrentAllocSize ());
  }

  MemorySize CacheEntity::getMaxSize () const {
    return this-> _maxSize;
  }

  MemorySize CacheEntity::getSize() const {
    return MemorySize::B (this-> _entity-> getPool (this-> _pool).getPoolSize ());
  }

  MemorySize CacheEntity::getSlabSize () {
    return MemorySize::B (cachelib::Slab::kSize);
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * ======================================      INSERT/FIND      ===============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  std::string fillKey (const std::string & key, uint64_t len) {
    std::stringstream ss;
    ss << key;
    for (uint64_t i = key.length () ; i < len ; i++) {
      ss << "X";
    }

    return ss.str ();
  }

  bool  CacheEntity::insert (const std::string & key, rd_utils::net::TcpStream& session) {
    auto valLen = session.receiveU32 ();
    if (valLen > this-> _maxValueSize.bytes ()) {
      LOG_ERROR ("Value for key : ", key, " too large : ", valLen);
      session.close ();
      return false;
    }

    try {
      auto toAlloc = MemorySize::nextPow2 (MemorySize::B (valLen + sizeof (int)));
      auto handle = this-> _entity-> allocate (this-> _pool, key.c_str (), toAlloc.bytes (), this-> _ttl);

      if (!handle) {
        for (uint64_t i = 0 ; i <= 32 ; i++) {
          toAlloc = MemorySize::nextPow2 (MemorySize::B (valLen + sizeof (int)) * i);
          handle = this-> _entity-> allocate (this-> _pool, key.c_str (), toAlloc.bytes (), this-> _ttl);
          if (handle != nullptr) break;
        }
      }

      if (!handle) {
        LOG_ERROR ("Cannot allocate, cache is full even after retry : ", valLen + sizeof (int), " ", toAlloc.bytes (), " ", key.c_str (), " ", key.size (), " ", this-> _pool, " ", this-> _ttl, " ", this);
        session.sendI32 (0);
        return false;
      }

      LOG_INFO ("Inserted : ", toAlloc.bytes (), " ", key, " ", key.size (), " ", this-> getCurrentMemoryUsage ().bytes (), " ", this-> getSize ().bytes (), " ", this-> _pool, " ", this);
      session.receiveRaw (static_cast <char*> (handle-> getMemory ()) + sizeof (int), valLen);
      this-> _entity-> insertOrReplace (handle);
      static_cast<int*> (handle-> getMemory ())[0] = valLen;

      session.sendI32 (1);
      return true;
    } catch (const std::exception & e) {
      LOG_ERROR ("Failed to insert key : ", key, " because ", e.what ());
      this-> _entity-> remove (key);
      session.close ();
      return false;
    }
  }

  bool CacheEntity::find (const std::string & key, rd_utils::net::TcpStream& session) {
    try {
      auto handle = this-> _entity-> find (key.c_str ());
      if (handle != nullptr) {
        size_t valLen = static_cast <const int*> (handle-> getMemory ())[0];
        session.sendU32 (valLen);
        session.sendRaw (static_cast <const char*> (handle-> getMemory ()) + sizeof (int), valLen);
        return true;
      } else {
        session.sendI32 (0);
        return false;
      }
    } catch (std::exception & e) {
      LOG_ERROR ("Failed to find key : ", key, " because ", e.what ());
      session.close ();
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
    if (this-> _entity != nullptr) {
      this-> _entity.reset ();
      this-> _entity = nullptr;
      this-> _pool = 0;
      this-> _maxValueSize = MemorySize::B (0);
      this-> _maxSize = MemorySize::B (0);
      this-> _requested = MemorySize::B (0);
      this-> _name = "";
    }
  }

  CacheEntity::~CacheEntity () {
    this-> dispose ();
  }

}
