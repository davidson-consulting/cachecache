#define LOG_LEVEL 10
#define __PROJECT__ "SOCIAL_GRAPH"

#include "../../utils/jwt/check.hh"
#include "service.hh"
#include "../../registry/service.hh"

using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet::social_graph {

  SocialGraphService::SocialGraphService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf) :
    actor::ActorBase (name, sys)
  {
    this-> _db.configure (conf);

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    this-> _secret = conf ["auth"]["secret"].getStr ();
    this-> _issuer = conf ["auth"].getOr ("issuer", "auth0");

    socialNet::registerService (this-> _registry, "social_graph", name, sys-> port (), this-> _iface);
  }

  void SocialGraphService::onQuit () {
    socialNet::closeService (this-> _registry, "social_graph", this-> _name, this-> _system-> port (), this-> _iface);
  }


  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> SocialGraphService::onRequest (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", "none");
    if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
      if (type == "sub") {
        return this-> subscribe (msg);
      } else if (type == "unsub") {
        return this-> deleteSub (msg);
      } else if (type == "count-sub") {
        return this-> countSubs (msg);
      } else if (type == "count-foll") {
        return this-> countFollows (msg);
      } else {
        return ResponseCode (404);
      }
    } else return ResponseCode (403);
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::subscribe (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t userId = msg ["userId"].getI ();
      uint32_t toWhom = msg ["toWhom"].getI ();
      auto sub = Sub {.userId = userId, .toWhom = toWhom};

      if (!this-> _db.findSub (sub)) {
        this-> _db.insertSub (sub);
      }

      return ResponseCode (200);
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    return ResponseCode (400);
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::deleteSub (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t userId = msg ["userId"].getI ();
      uint32_t toWhom = msg ["toWhom"].getI ();
      auto sub = Sub {.userId = userId, .toWhom = toWhom};
      this-> _db.removeSub (sub);

      return ResponseCode (200);
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
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


  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::countSubs (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto userId = msg ["userId"].getI ();
      auto cnt = this-> _db.countNbSubs (userId);
      return ResponseCode (200, std::make_shared <config::Int> (cnt));
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    return ResponseCode (400);
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::countFollows (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto userId = msg ["userId"].getI ();
      auto cnt = this-> _db.countNbFollowers (userId);
      return ResponseCode (200, std::make_shared <config::Int> (cnt));
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
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


  void SocialGraphService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    auto type = msg.getOr ("type", "none");
    if (type == "followers") {
      this-> streamFollowers (msg, stream);
    } else if (type == "subscriptions") {
      this-> streamSubs (msg, stream);
    } else {
      stream.writeU8 (0);
    }
  }

  void SocialGraphService::streamFollowers (const config::ConfigNode & msg, actor::ActorStream & stream) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      uint32_t page = msg.getOr ("page", -1);
      uint32_t nb = msg.getOr ("nb", -1);

      uint32_t rid;
      auto statement = this-> _db.prepareFindFollowers (&rid, uid, page, nb);
      statement-> execute ();
      while (statement-> next () && stream.isOpen ()) {
        stream.writeU8 (1);
        stream.writeU32 (rid);
      }
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    stream.writeU8 (0);
  }

  void SocialGraphService::streamSubs (const config::ConfigNode & msg, actor::ActorStream & stream) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      uint32_t page = msg.getOr ("page", -1);
      uint32_t nb = msg.getOr ("nb", -1);

      uint32_t rid;
      auto statement = this-> _db.prepareFindSubscriptions (&rid, uid, page, nb);
      statement-> execute ();
      while (statement-> next () && stream.isOpen ()) {
        stream.writeU8 (1);
        stream.writeU32 (rid);
      }
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    stream.writeU8 (0);
  }

}
