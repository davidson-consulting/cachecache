#define LOG_LEVEL 10
#define __PROJECT__ "USER"

#include "service.hh"
#include "../../registry/service.hh"

using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet::user {

  UserService::UserService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf) :
    actor::ActorBase (name, sys)
  {
    this-> _db.configure (conf);

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    socialNet::registerService (this-> _registry, "user", name, sys-> port (), this-> _iface);
  }


  void UserService::onQuit () {
    socialNet::closeService (this-> _registry, "user", this-> _name, this-> _system-> port (), this-> _iface);
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> UserService::onRequest (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", "none");
    LOG_INFO ("type : ", type);

    if (type == "register") {
      return this-> registerUser (msg);
    } else if (type == "login") {
      return this-> login (msg);
    } else if (type == "find") {
      return this-> find (msg);
    } else {
      return ResponseCode (404);
    }
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> UserService::registerUser (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto login = msg ["login"].getStr ();
      auto password = msg ["password"].getStr ();

      rd_utils::memory::cache::collection::FlatString <16> lg = login;
      User u;
      if (this-> _db.findByLogin (lg, u)) {
        return ResponseCode (403);
      }

      u.login = login;
      u.password = password;
      auto id = this-> _db.insertUser (u);
      return ResponseCode (200, std::make_shared <config::Int> (id));
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR UserService::registerUser : ", err.what ());
    }

    return ResponseCode (400);
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          FINDING         ========================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <rd_utils::utils::config::ConfigNode> UserService::login (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto login = msg ["login"].getStr ();
      auto password = msg ["password"].getStr ();

      LOG_INFO ("Trying to connect : ", login, "@", password);

      User u;
      rd_utils::memory::cache::collection::FlatString <16> lg = login;
      if (this-> _db.findByLogin (lg, u)) {
        if (u.password == password) {
          return ResponseCode (200, std::make_shared <config::Int> (u.id));
        } else {
          LOG_ERROR ("Wrong password");
          return ResponseCode (403);
        }
      } else {
        LOG_ERROR ("Wrong username");
        return ResponseCode (403);
      }
    } catch (std::runtime_error & err) {
      LOG_ERROR ("UserService::login : ", err.what ());
    }

    return ResponseCode (400);
  }


  std::shared_ptr <rd_utils::utils::config::ConfigNode> UserService::find (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      User u;
      if (msg.contains ("id")) {
        uint32_t id = msg ["id"].getI ();
        if (this-> _db.findById (id, u)) {
          return ResponseCode (200, std::make_shared <config::String> (u.login.data ()));
        }
      } else {
        auto login = msg ["login"].getStr ();
        rd_utils::memory::cache::collection::FlatString <16> lg = login;
        if (this-> _db.findByLogin (lg, u)) {
          return ResponseCode (200, std::make_shared <config::Int> (u.id));
        }
      }
    }  catch (std::runtime_error & err) {
      LOG_INFO ("ERROR UserService::find : ", err.what ());
    }

    return ResponseCode (400);
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          STREAMING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void UserService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    auto type = msg.getOr ("type", "none");
    if (type == "find_by_login") {
      this-> streamFindByLogin (stream);
    } else if (type == "find_by_id") {
      this-> streamFindById (stream);
    } else {
      stream.writeU8 (0);
    }
  }

  void UserService::streamFindByLogin (actor::ActorStream & stream) {
    User u;
    while (stream.isOpen ()) {
      auto n = stream.readOr (0);
      if (n != 0) {
        auto login = stream.readStr ();
        rd_utils::memory::cache::collection::FlatString <16> lg = login;
        if (this-> _db.findByLogin (lg, u)) {
          stream.writeU8 (1);
          stream.writeU32 (u.id);
        } else {
          stream.writeU8 (0);
        }
      } else break;
    }
  }

  void UserService::streamFindById (actor::ActorStream & stream) {
    User u;
    while (stream.isOpen ()) {
      if (stream.readOr (0) != 0) {
        auto id = stream.readU32 ();
        if (this-> _db.findById (id, u)) {
          stream.writeU8 (1);
          stream.writeStr (std::string (u.login.data ()));
        } else {
          stream.writeU8 (0);
        }
      } else break;
    }
  }



}
