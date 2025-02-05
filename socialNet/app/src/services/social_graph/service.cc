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

    CONF_LET (dbName, conf ["services"]["social"]["db"].getStr (), std::string ("mysql"));
    CONF_LET (chName, conf ["services"]["social"]["cache"].getStr (), std::string (""));

    this-> _db.configure (dbName, chName, conf);

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    this-> _secret = conf ["auth"]["secret"].getStr ();
    this-> _issuer = conf ["auth"].getOr ("issuer", "auth0");
  }

  void SocialGraphService::onStart () {
    socialNet::registerService (this-> _registry, "social_graph", this-> _name, this-> _system-> port (), this-> _iface);
  }

  void SocialGraphService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _registry = nullptr;
        this-> exit ();
      }

      fo;
    }
  }

  void SocialGraphService::onQuit () {
    if (this-> _registry != nullptr) {
      socialNet::closeService (this-> _registry, "social_graph", this-> _name, this-> _system-> port (), this-> _iface);
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

  std::shared_ptr<config::ConfigNode> SocialGraphService::onRequest (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::FOLLOWERS_LEN) {
        return this-> countFollows (msg);
      }

      elof_v (RequestCode::SUBS_LEN) {
        return this-> countSubs (msg);
      }

      elof_v (RequestCode::SUBSCRIBE) {
        if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
          return this-> subscribe (msg);
        } else return response (ResponseCode::FORBIDDEN);
      }

      elof_v (RequestCode::UNSUBSCRIBE) {
        if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
          return this-> deleteSub (msg);
        } else return response (ResponseCode::FORBIDDEN);
      }

      elof_v (RequestCode::IS_FOLLOWING) {
        return this-> isFollowing (msg);
      }

      elof_v (RequestCode::FOLLOWERS) {
        return this-> readFollowers (msg);
      }

      elof_v (RequestCode::SUBSCIRPTIONS) {
        return this-> readSubscriptions (msg);
      }


      elfo {
        return response (ResponseCode::NOT_FOUND);
      }
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          SUBSCRIBING          ===================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::subscribe (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t userId = msg ["userId"].getI ();
      uint32_t toWhom = msg ["toWhom"].getI ();
      if (userId == toWhom) {
        LOG_INFO ("Throw 16");
        throw std::runtime_error ("Cannot subscribe to self");
      }

      auto sub = Sub {.userId = userId, .toWhom = toWhom};

      if (!this-> _db.findSub (sub)) {
        this-> _db.insertSub (sub);
      }

      return response (ResponseCode::OK);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("SocialGraphService::subscribe : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::deleteSub (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t userId = msg ["userId"].getI ();
      uint32_t toWhom = msg ["toWhom"].getI ();
      if (userId == toWhom) return response (ResponseCode::OK);

      auto sub = Sub {.userId = userId, .toWhom = toWhom};
      this-> _db.removeSub (sub);

      return response (ResponseCode::OK);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("SocialGraphService::deleteSub : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::isFollowing (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t userId = msg ["userId"].getI ();
      uint32_t toWhom = msg ["toWhom"].getI ();
      if (userId == toWhom) return response (ResponseCode::OK, 0);

      auto sub = Sub {.userId = userId, .toWhom = toWhom};
      if (this-> _db.findSub (sub)) {
        return response (ResponseCode::OK, 1);
      } else {
        return response (ResponseCode::OK, 0);
      }
    } catch (std::runtime_error & err) {
      LOG_ERROR ("SocialGraphService::deleteSub : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          COUNTING          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */


  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::countSubs (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto userId = msg ["userId"].getI ();
      auto cnt = this-> _db.countNbSubs (userId);

      return response (ResponseCode::OK,
                           std::make_shared <config::Int> (cnt));
    } catch (std::runtime_error & err) {
      LOG_ERROR ("SocialGraphService::countSubs : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr <rd_utils::utils::config::ConfigNode> SocialGraphService::countFollows (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      auto userId = msg ["userId"].getI ();
      auto cnt = this-> _db.countNbFollowers (userId);

      return response (ResponseCode::OK,
                           std::make_shared <config::Int> (cnt));
    } catch (std::runtime_error & err) {
      LOG_ERROR ("SocialGraphService::countFollows : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          READING          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> SocialGraphService::readFollowers (const config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      int32_t nb = msg ["nb"].getI ();
      int32_t page = msg ["page"].getI ();

      auto result = std::make_shared <config::Array> ();
      auto values = this-> _db.findFollowersCacheable (uid, page * nb, nb);
      for (auto & it : values) {
        result-> insert (std::make_shared <config::Int> (it));
      }

      return response (ResponseCode::OK, result);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("SocialGraphService::readFollowers : ", err.what (), " ", msg);
    }

    return response (ResponseCode::MALFORMED);
  }

  std::shared_ptr<config::ConfigNode> SocialGraphService::readSubscriptions (const config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      int32_t nb = msg ["nb"].getI ();
      int32_t page = msg ["page"].getI ();

      auto result = std::make_shared <config::Array> ();
      auto values = this-> _db.findSubsCacheable (uid, page * nb, nb);
      for (auto & it : values) {
        result-> insert (std::make_shared <config::Int> (it));
      }

      return response (ResponseCode::OK, result);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("SocialGraphService::readSubscriptions : ", err.what ());
    }

    return response (ResponseCode::MALFORMED);
  }

}
