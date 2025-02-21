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

  void CacheEntity::configure (const std::string & name, rd_utils::utils::MemorySize maxSize, uint32_t slabTTL) {
    uint32_t nbSlabs = maxSize.bytes () / kv_store::common::KVMAP_SLAB_SIZE.bytes ();
    this-> _entity = std::make_unique <HybridKVStore> (nbSlabs, slabTTL);

    this-> _traces_operations = std::make_shared <rd_utils::utils::trace::CsvExporter> ("/tmp/operations_cache_" + name + ".csv");
    this->_start = std::chrono::system_clock::now();

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

      WITH_LOCK (this->_m) {
	config::Dict d;
	d.insert("key", key);
	d.insert("key_size", (int64_t) key.size());
	d.insert("value_size", (int64_t) valLen);
	d.insert("client_id", (int64_t) 0);
	d.insert("operation", "set");
	d.insert("ttl", (int64_t) 0);

	const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - this->_start);
	this->_traces_operations->append((uint32_t) duration.count(), d);
      }

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

  bool CacheEntity::find (const std::string & key, rd_utils::net::TcpStream& session) {
    try {
      common::Key k;
      k.set (key);

      WITH_LOCK (this->_m) {
	config::Dict d;
	d.insert("key", key);
	d.insert("key_size", (int64_t) key.size());
	d.insert("value_size", (int64_t) 0);
	d.insert("client_id", (int64_t) 0);
	d.insert("operation", "get");
	d.insert("ttl", (int64_t) 0);

	const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - this->_start);
	this->_traces_operations->append((uint32_t) duration.count(), d);
      }

      auto value = this-> _entity-> find (k);
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
