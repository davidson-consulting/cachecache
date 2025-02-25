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
    , _conf (conf)
  {
    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
  }

  void UserService::onStart () {
    CONF_LET (dbName, this-> _conf ["services"]["user"]["db"].getStr (), std::string ("mysql"));
    CONF_LET (chName, this-> _conf ["services"]["user"]["cache"].getStr (), std::string (""));

    this-> _uid = socialNet::registerService (this-> _registry, "user", this-> _name, this-> _system-> port (), this-> _iface);

    try {
      this-> _db.configure (dbName, chName, this-> _conf);
    }  catch (const std::runtime_error & err) {
      LOG_ERROR ("Failed to connect to DB", err.what ());
      socialNet::closeService (this-> _registry, "user", this-> _name, this-> _system-> port (), this-> _iface);
      this-> _registry = nullptr;
      throw err;
    }

    this-> _conf = config::Dict ();
  }

  void UserService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _needClose = false;
        this-> exit ();
      }

      fo;
    }
  }

  void UserService::onQuit () {
    if (this-> _registry != nullptr && this-> _needClose) {
      socialNet::closeService (this-> _registry, "user", this-> _name, this-> _system-> port (), this-> _iface);
      this-> _registry = nullptr;
    }

    this-> _db.dispose ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          REQUESTS          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> UserService::onRequest (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::REGISTER_USER) {
        return this-> registerUser (msg);
      }

      elof_v (RequestCode::LOGIN_USER) {
        return this-> login (msg);
      }

      elof_v (RequestCode::FIND) {
        return this-> find (msg);
      }

      elfo {
        return response (ResponseCode::NOT_FOUND);
      }
    }
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> UserService::registerUser (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto login = msg ["login"].getStr ();
      auto password = msg ["password"].getStr ();

      User u;
      if (this-> _db.findByLogin (login, u)) {
        return response (ResponseCode::FORBIDDEN);
      }

      memcpy (u.login, login.c_str (), strnlen (login.c_str (), 16));
      memcpy (u.password, password.c_str (), strnlen (password.c_str (), 64));

      uint32_t id = this-> _db.insertUser (u);
      return response (ResponseCode::OK, std::make_shared <config::Int> (id));
    } catch (std::runtime_error & err) {
      LOG_ERROR ("ERROR UserService::registerUser : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
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

      LOG_DEBUG ("Trying to connect : ", login, "@", password);

      User u;
      if (this-> _db.findByLogin (login, u)) {
        if (strnlen (u.password, 64) == password.length ()) {
          if (strncmp (u.password, password.c_str (), password.length ()) == 0) {
            LOG_DEBUG ("User connected");
            return response (ResponseCode::OK, std::make_shared <config::Int> (u.id));
          }
        }

        LOG_ERROR ("Wrong password");
        return response (ResponseCode::FORBIDDEN);
      } else {
        LOG_ERROR ("Wrong username");
        return response (ResponseCode::FORBIDDEN);
      }
    } catch (std::runtime_error & err) {
      LOG_ERROR ("UserService::login : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }


  std::shared_ptr <rd_utils::utils::config::ConfigNode> UserService::find (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      User u;
      if (msg.contains ("id")) {
        uint32_t id = msg ["id"].getI ();
        if (this-> _db.findById (id, u)) {
          return response (ResponseCode::OK, std::make_shared <config::String> (u.login));
        } else {
          LOG_ERROR ("Not found : ", msg);
        }
      } else {
        auto login = msg ["login"].getStr ();
        if (this-> _db.findByLogin (login, u)) {
          return response (ResponseCode::OK, std::make_shared <config::Int> (u.id));
        }
      }

      return response (ResponseCode::NOT_FOUND);
    }  catch (std::runtime_error & err) {
      LOG_ERROR ("ERROR UserService::find : ", err.what (), " => ", msg);
      return response (ResponseCode::MALFORMED);
    }
  }

}
