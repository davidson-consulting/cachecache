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

  void UserService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _registry = nullptr;
        this-> exit ();
      }

      fo;
    }
  }

  void UserService::onQuit () {
    if (this-> _registry != nullptr) {
      socialNet::closeService (this-> _registry, "user", this-> _name, this-> _system-> port (), this-> _iface);
    }
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
      //WITH_LOCK (this-> getMutex ()) {
        if (this-> _db.findByLogin (login, u)) {
          return response (ResponseCode::FORBIDDEN);
        }
      //}

      memcpy (u.login, login.c_str (), strnlen (login.c_str (), 16));
      memcpy (u.password, password.c_str (), strnlen (password.c_str (), 64));

      uint32_t id = 0;
      //WITH_LOCK (this-> getMutex ()) {
        id = this-> _db.insertUser (u);
      //}

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
      bool found = false;
      //WITH_LOCK (this-> getMutex ()) {
        found = this-> _db.findByLogin (login, u);
      //}

      if (found) {
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
        bool found = false;
        //WITH_LOCK (this-> getMutex ()) {
          found = this-> _db.findById (id, u);
        //}

        if (found) {
          return response (ResponseCode::OK, std::make_shared <config::String> (u.login));
        } else {
          LOG_ERROR ("Not found : ", msg);
        }
      } else {
        auto login = msg ["login"].getStr ();
        bool found = false;
        //WITH_LOCK (this-> getMutex ()) {
          found = this-> _db.findByLogin (login, u);
        //}

        if (found) {
          return response (ResponseCode::OK, std::make_shared <config::Int> (u.id));
        }
      }

      return response (ResponseCode::NOT_FOUND);

    }  catch (std::runtime_error & err) {
      LOG_ERROR ("ERROR UserService::find : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          STREAMING          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void UserService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::FIND_BY_LOGIN) {
        this-> streamFindByLogin (stream);
      }
      elof_v (RequestCode::FIND_BY_ID) {
        this-> streamFindById (stream);
      }

      elfo {
        stream.writeU32 (ResponseCode::NOT_FOUND);
      }
    }
  }

  void UserService::streamFindByLogin (actor::ActorStream & stream) {
    try {
      User u;
      stream.writeU32 (200);
      while (stream.readOr (0) == 1) {
        auto login = stream.readStr ();
        bool found = false;
        //WITH_LOCK (this-> getMutex ()) {
          found = this-> _db.findByLogin (login, u);
        //}

        if (found) {
          stream.writeU8 (1);
          stream.writeU32 (u.id);
        } else {
          stream.writeU8 (0);
        }
      }
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("UserService::streamFindByLogin ", err.what ());
    }
  }

  void UserService::streamFindById (actor::ActorStream & stream) {
    try {
      User u;
      stream.writeU32 (200);
      while (stream.readOr (0) == 1) {
        auto id = stream.readU32 ();
        bool found = false;
        //WITH_LOCK (this-> getMutex ()) {
          found = this-> _db.findById (id, u);
        //}

        if (found) {
          stream.writeU8 (1);
          stream.writeStr (std::string (u.login));
        } else {
          stream.writeU8 (0);
        }
      }
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("UserService::streamFindById ", err.what ());
    }
  }



}
