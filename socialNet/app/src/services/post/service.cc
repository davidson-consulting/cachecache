#define __PROJECT__ "POST"

#include "../../utils/jwt/check.hh"
#include "service.hh"
#include "../../registry/service.hh"

using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet::post {

  PostStorageService::PostStorageService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf) :
    actor::ActorBase (name, sys)
  {
    CONF_LET (dbName, conf["services"]["post"]["db"].getStr (), std::string ("mysql"));
    CONF_LET (chName, conf["services"]["post"]["cache"].getStr (), std::string (""));

    try {
      this-> _db.configure (dbName, chName, conf);
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("Failed to connect to DB", err.what ());
      throw err;
    }

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    this-> _secret = conf ["auth"]["secret"].getStr ();
    this-> _issuer = conf ["auth"].getOr ("issuer", "auth0");
  }

  void PostStorageService::onStart () {
    socialNet::registerService (this-> _registry, "post", this-> _name, this-> _system-> port (), this-> _iface);
  }

  void PostStorageService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _registry = nullptr;
        this-> exit ();
      }

      fo;
    }
  }

  void PostStorageService::onQuit () {
    if (this-> _registry != nullptr) {
      socialNet::closeService (this-> _registry, "post", this-> _name, this-> _system-> port (), this-> _iface);
      this-> _registry = nullptr;
    }

    this-> _db.dispose ();
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> PostStorageService::onRequest (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::STORE) {
        if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
          return this-> store (msg);
        } else {
          return response (ResponseCode::FORBIDDEN);
        }
      }

      elof_v (RequestCode::READ_POST) {
        return this-> readOne (msg);
      }

      elfo {
        return response (ResponseCode::NOT_FOUND);
      }
    }
  }

  std::shared_ptr<config::ConfigNode> PostStorageService::store (const config::ConfigNode & msg) {
    try {
      auto timeline = socialNet::findService (this-> _system, this-> _registry, "timeline");
      if (timeline == nullptr) return response (ResponseCode::SERVER_ERROR);

      auto userId = msg ["userId"].getI ();
      auto userLogin = msg ["userLogin"].getStr ();
      auto text = msg ["text"].getStr ();
      uint32_t tags [MAX_TAGS];
      uint32_t nbTags = 0;

      match (msg ["tags"]) {
        of (config::Array, a) {
          for (uint32_t i = 0 ; i < a-> getLen () && i < MAX_TAGS ; i++) {
            tags [i] = (*a) [i].getI ();
            nbTags += 1;
          }
        } fo;
      }

      auto pid = this-> _db.insertPost (userId, userLogin, text, tags, nbTags);
      auto req = config::Dict ()
        .insert ("type", RequestCode::UPDATE_TIMELINE)
        .insert ("userId", (int64_t) userId)
        .insert ("jwt_token", msg ["jwt_token"].getStr ())
        .insert ("tags", msg.get ("tags"))
        .insert ("postId", (int64_t) pid);

      // Asynchronous timeline update
      timeline-> send (req);


      return response (ResponseCode::OK);
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("PostStorageService::store : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr <config::ConfigNode> PostStorageService::readOne (const config::ConfigNode & msg) {
    try {
      Post p;
      if (this-> _db.findPost (msg ["postId"].getI (), p)) {
        auto result = std::make_shared<config::Dict> ();
        result-> insert ("userId", std::make_shared <config::Int> (p.userId));
        result-> insert ("userLogin", std::string (p.userLogin));
        result-> insert ("text", std::string (p.text));
        result-> insert ("postId", msg.get ("postId"));

        auto tags = std::make_shared <config::Array> ();
        for (uint32_t i = 0 ; i < p.nbTags && i < 16 ; i++) {
          tags-> insert (std::make_shared <config::Int> (p.tags [i]));
        }
        result-> insert ("tags", tags);

        return response (ResponseCode::OK, result);
      } else {
        return response (ResponseCode::NOT_FOUND);
      }
    } catch (...) {
      return response (ResponseCode::MALFORMED);
    }
  }

}
