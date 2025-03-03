#include "base.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::social_graph {

  SocialGraphDatabase::SocialGraphDatabase () :
    _cache (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void SocialGraphDatabase::configureCache (const std::string & ch, const config::ConfigNode & conf) {
    if (conf.contains ("cache") && ch != "") {
      auto cacheAddr = conf["cache"][ch].getOr ("addr", "localhost");
      auto cachePort = conf["cache"][ch].getOr ("port", 6650);
      this-> _cache = std::make_shared <socialNet::utils::CacheClient> (cacheAddr, cachePort);
    }
  }

  void SocialGraphDatabase::dispose () {
    this-> _cache.reset ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          CACHE          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */


  bool SocialGraphDatabase::findSubsInCache (std::vector <uint32_t> & res, uint32_t uid, uint32_t page, uint32_t nb) {
    if (this-> _cache == nullptr) return false;
    if (this-> _cache-> get ("sub_t/" + std::to_string (uid) + "/" + std::to_string (page) + "/" + std::to_string (nb), res)) {
      return true;
    }

    return false;
  }

  void SocialGraphDatabase::insertSubsInCache (const std::vector <uint32_t> & res, uint32_t uid, uint32_t page, uint32_t nb) {
    if (this-> _cache == nullptr) return;

    auto log =  "sub_t/" + std::to_string (uid) + "/" + std::to_string (page) + "/" + std::to_string (nb);
    this-> _cache-> set (log, reinterpret_cast <const uint8_t*> (res.data ()), res.size () * sizeof (uint32_t));
  }

  bool SocialGraphDatabase::findFollowersInCache (std::vector <uint32_t> & res, uint32_t uid, uint32_t page, uint32_t nb) {
    if (this-> _cache == nullptr) return false;
    if (this-> _cache-> get ("fol_t/" + std::to_string (uid) + "/" + std::to_string (page) + "/" + std::to_string (nb), res)) {
      return true;
    }

    return false;
  }

  void SocialGraphDatabase::insertFollowersInCache (const std::vector <uint32_t> & res, uint32_t uid, uint32_t page, uint32_t nb) {
    if (this-> _cache == nullptr  || res.size () <= 1) return;

    auto log =  "fol_t/" + std::to_string (uid) + "/" + std::to_string (page) + "/" + std::to_string (nb);
    this-> _cache-> set (log, reinterpret_cast <const uint8_t*> (res.data ()), res.size () * sizeof (uint32_t));
  }

}
