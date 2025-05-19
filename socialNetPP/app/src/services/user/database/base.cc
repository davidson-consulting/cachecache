#include "base.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::user {

  UserDatabase::UserDatabase () :
    _cache (nullptr)
  {}

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          CONFIGURATION          ==================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void UserDatabase::configureCache (const std::string & ch, const config::ConfigNode & conf) {
    if (conf.contains ("cache") && ch != "") {
      auto cacheAddr = conf["cache"][ch].getOr ("addr", "localhost");
      auto cachePort = conf["cache"][ch].getOr ("port", 6650);
      this-> _cache = std::make_shared <socialNet::utils::CacheClient> (cacheAddr, cachePort);
    }
  }

  void UserDatabase::dispose () {
    this-> _cache.reset ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          CACHE          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  bool UserDatabase::findByIdInCache (uint32_t id, User & u) {
    if (this-> _cache == nullptr) return false;
    if (this-> _cache-> get ("user-i/" + std::to_string (id), reinterpret_cast <uint8_t*> (&u), sizeof (User))) {
      return true;
    }

    return false;
  }

  void UserDatabase::insertByIdInCache (User & u) {
    if (this-> _cache == nullptr) return;

    std::string log = "user-i/";
    this-> _cache-> set (log  + std::to_string (u.id), reinterpret_cast<const uint8_t*> (&u), sizeof (User));
  }

  bool UserDatabase::findByLoginInCache (const std::string & login, User & u) {
    if (this-> _cache == nullptr) return false;
    if (this-> _cache-> get ("user-l/" + login, reinterpret_cast <uint8_t*> (&u), sizeof (User))) {
      return true;
    }

    return false;
  }

  void UserDatabase::insertByLoginInCache (User & u) {
    if (this-> _cache == nullptr) return;

    auto log = "user-l/" + std::string (u.login);
    this-> _cache-> set (log, reinterpret_cast<const uint8_t*> (&u), sizeof (User));
  }

}
