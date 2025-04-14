#define LOG_LEVEL 10
#include "entity.hh"
#include "../cache/common/_.hh"

using namespace rd_utils::utils;

namespace kv_store::instance {

  CacheEntity::CacheEntity ()
    : _maxSize (MemorySize::B (0))
    , _entity (nullptr)
  {}

  /**
   * ============================================================================================================
   * ============================================================================================================
   * =========================================      CONFIGURE     ===============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  void CacheEntity::configure (const std::string & name, rd_utils::utils::MemorySize maxSize, rd_utils::utils::MemorySize diskSize, uint32_t slabTTL) {
    uint32_t nbSlabs = maxSize.bytes () / kv_store::common::KVMAP_SLAB_SIZE.bytes ();
    uint32_t maxDiskSlabs = diskSize.bytes () / kv_store::common::KVMAP_SLAB_SIZE.bytes ();
    this-> _entity = std::make_unique <HybridKVStore> (nbSlabs, maxDiskSlabs, slabTTL);
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * =========================================      RESIZE      =================================================
   * ============================================================================================================
   * ============================================================================================================
   */

  void CacheEntity::loop () {
    this-> _entity-> loop ();
  }

  bool CacheEntity::resize (rd_utils::utils::MemorySize newSize) {
    if (this-> _entity == nullptr) return false;
    uint32_t nbSlabs = newSize.bytes () / kv_store::common::KVMAP_SLAB_SIZE.bytes ();

    this-> _entity-> resizeRamColl (nbSlabs);
    return true;
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
    return this-> _entity-> getRamColl ().getMemoryUsage ();
  }

  MemorySize CacheEntity::getCurrentDiskUsage () const {
    if (this-> _entity == nullptr) return MemorySize::B (0);
    return this-> _entity-> getDiskColl ().getMemoryUsage ();
  }

  MemorySize CacheEntity::getMaxSize () const {
    return this-> _maxSize;
  }

  MemorySize CacheEntity::getSize() const {
    return this-> _entity-> getRamColl ().getMemorySize ();
  }

  /**
   * ============================================================================================================
   * ============================================================================================================
   * ======================================      INSERT/FIND      ===============================================
   * ============================================================================================================
   * ============================================================================================================
   */

  bool  CacheEntity::insert (const std::string & key, rd_utils::net::TcpStream& session) {
    try {
      auto valLen = session.receiveU32 ();
      common::Key k;
      k.set (key);

      common::Value v (valLen);
      session.receiveRaw (v.data (), valLen);

      this-> _entity-> insert (k, v);
      session.sendI32 (1);
      return true;
    } catch (const std::exception & e) {
       LOG_ERROR ("Failed to insert key : ", key, " because ", e.what ());
       common::Key k;
       k.set (key);

       this-> _entity-> remove (k);
       // session.close ();
       return false;
    }
  }

  bool CacheEntity::find (const std::string & key, rd_utils::net::TcpStream& session, bool & onDisk) {
    try {
      common::Key k;
      k.set (key);

      auto value = this-> _entity-> find (k, onDisk);
      if (value != nullptr) {
        session.sendU32 (value-> len ());
        session.sendRaw (value-> data (), value-> len ());
        return true;
      } else {
        session.sendI32 (0);
        return false;
      }
    } catch (std::exception & e) {
      LOG_ERROR ("Failed to find key : ", key, " because ", e.what ());
      // session.close ();
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
      this-> _maxSize = MemorySize::B (0);
      this-> _name = "";
    }
  }

  CacheEntity::~CacheEntity () {
    this-> dispose ();
  }

}
