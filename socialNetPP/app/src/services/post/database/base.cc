#include "base.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::post {

  PostDatabase::PostDatabase () :
    _cache (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void PostDatabase::configureCache (const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) {
    if (conf.contains ("cache") && ch != "") {
      auto cacheAddr = conf["cache"][ch].getOr ("addr", "localhost");
      auto cachePort = conf["cache"][ch].getOr ("port", 6650);
      this-> _cache = std::make_shared <socialNet::utils::CacheClient> (cacheAddr, cachePort);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          CACHE          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  bool PostDatabase::findPostInCache (uint32_t id, Post & p) {
    if (this-> _cache == nullptr) return false;
    if (this-> _cache-> get ("post/" + std::to_string (id), reinterpret_cast <uint8_t*> (&p), sizeof (Post))) {
      return true;
    }

    return false;
  }

  void PostDatabase::insertPostInCache (uint32_t id, Post & p) {
    if (this-> _cache == nullptr) return;

    auto log = "post/" + std::to_string (id);
    this-> _cache-> set (log, reinterpret_cast<const uint8_t*> (&p), sizeof (Post));
  }

  void PostDatabase::dispose () {
    this-> _cache.reset ();
  }

}

std::ostream& operator<< (std::ostream & s, const socialNet::post::Post & p) {
  s << "Post (" << p.userId << "," << p.userLogin << "," << p.text << ", [";
  for (uint32_t i = 0 ; i < p.nbTags ; i++) {
    if (i != 0) s << ", ";
    s << p.tags [i];
  }
  s << "])";
  return s;
}
