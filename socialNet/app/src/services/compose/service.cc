#define __PROJECT__ "COMPOSE"

#include "../../utils/jwt/check.hh"
#include "service.hh"
#include "../../registry/service.hh"
#include "../post/database.hh"



using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet::compose {

  ComposeService::ComposeService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode & conf) :
    actor::ActorBase (name, sys)
  {
    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    this-> _secret = conf ["auth"]["secret"].getStr ();
    this-> _issuer = conf ["auth"].getOr ("issuer", "auth0");
  }

  void ComposeService::onStart () {
    socialNet::registerService (this-> _registry, "compose", this-> _name, this-> _system-> port (), this-> _iface);
  }

  void ComposeService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _registry = nullptr;
        this-> exit ();
      }

      fo;
    }
  }

  void ComposeService::onQuit () {
    if (this-> _registry != nullptr) {
      socialNet::closeService (this-> _registry, "compose", this-> _name, this-> _system-> port (), this-> _iface);
    }
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */


  std::shared_ptr<config::ConfigNode> ComposeService::onRequest (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::SUBMIT_POST) {
        if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
          return this-> submitNewPost (msg);
        } else return response (ResponseCode::FORBIDDEN);
      }

      elof_v (RequestCode::REGISTER_USER) {
        return this-> registerUser (msg);
      }

      elof_v (RequestCode::LOGIN_USER) {
        return this-> login (msg);
      }

      elof_v (RequestCode::CHECK_CONNECTED) {
        if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
          return response (ResponseCode::OK, 1);
        } else {
          return response (ResponseCode::OK, 0);
        }
      }

      elof_v (RequestCode::HOME_TIMELINE) {
        return this-> readTimeline (msg, RequestCode::HOME_TIMELINE);
      }

      elof_v (RequestCode::USER_TIMELINE) {
        return this-> readTimeline (msg, RequestCode::USER_TIMELINE);
      }

      elof_v (RequestCode::SUBSCIRPTIONS) {
        return this-> readSubscriptions (msg, RequestCode::SUBSCIRPTIONS);
      }

      elof_v (RequestCode::FOLLOWERS) {
        return this-> readSubscriptions (msg, RequestCode::FOLLOWERS);
      }


      elfo {
        return response (ResponseCode::NOT_FOUND);
      }
    }
  }


  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          LOGIN/OUT         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> ComposeService::login (const config::ConfigNode & msg) {
    try {
      auto userService = socialNet::findService (this-> _system, this-> _registry, "user");
      config::Dict userReq = config::Dict ()
        .insert ("type", RequestCode::LOGIN_USER)
        .insert ("login", msg ["login"].getStr ())
        .insert ("password", msg ["password"].getStr ());

      auto result = userService-> request (userReq).wait ();
      if (result && result-> getOr ("code", -1) == ResponseCode::OK) {
        auto resp = config::Dict ()
          .insert ("userId", (*result) ["content"].getI ())
          .insert ("jwt_token", utils::createJWTToken ((*result) ["content"].getI (), this-> _issuer, this-> _secret));

        return response (ResponseCode::OK, resp);
      } else {
        LOG_ERROR ("No response, or wrong code ? ", __FILE__, __LINE__);
        return response (ResponseCode::FORBIDDEN);
      }
    } catch (std::runtime_error & err) {
      LOG_ERROR ("Compose::login, Error : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr<config::ConfigNode> ComposeService::registerUser (const config::ConfigNode & msg) {
    try {
      auto userService = socialNet::findService (this-> _system, this-> _registry, "user");
      auto userReq = config::Dict ()
        .insert ("type", RequestCode::REGISTER_USER)
        .insert ("login", msg ["login"].getStr ())
        .insert ("password", msg ["password"].getStr ());

      auto result = userService-> request (userReq).wait ();
      if (result && result-> getOr ("code", -1) == ResponseCode::OK) {
        return response (ResponseCode::OK);
      } else {
        return response (ResponseCode::FORBIDDEN);
      }
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("ComposeService::registerUser ", __FILE__, __LINE__, err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }


  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */




  std::shared_ptr <config::ConfigNode> ComposeService::submitNewPost (const config::ConfigNode & msg) {
    try {
      auto textService = socialNet::findService (this-> _system, this-> _registry, "text");
      auto userService = socialNet::findService (this-> _system, this-> _registry, "user");

      auto textReq = config::Dict ()
        .insert ("type", RequestCode::CTOR_MESSAGE)
        .insert ("text", msg ["content"].getStr ());

      auto userReq = config::Dict ()
        .insert ("type", RequestCode::FIND)
        .insert ("id", msg ["userId"].getI ());

      auto textResReq = textService-> request (textReq);
      auto userResReq = userService-> request (userReq);

      std::shared_ptr <config::ConfigNode> textResult;
      try {
        textResult = textResReq.wait ();
        if (textResult == nullptr || textResult-> getOr ("code", -1) != ResponseCode::OK) {
          LOG_INFO ("THROW 11");
          throw std::runtime_error ("Text incorrect");
        }
      } catch (const std::runtime_error & err) {
        LOG_ERROR ("ComposeService::submitNewPost : ", __FILE__, " ", __LINE__, " ", err.what ());
        return response (ResponseCode::MALFORMED);
      }

      auto arr = std::make_shared <config::Array> ();
      match ((*textResult) ["content"]["tags"]) {
        of (config::Array, a) {
          for (uint32_t i = 0 ; i < a-> getLen () && i < 16 ; i++) {
            arr-> insert ((*a) [i].getI ());
          }
        } fo;
      }

      std::shared_ptr <config::ConfigNode> userResult;
      try {
        userResult = userResReq.wait ();
        if (userResult == nullptr || userResult-> getOr ("code", -1) != ResponseCode::OK) {
          LOG_INFO ("THROW 13");
          throw std::runtime_error ("User not found");
        }
      } catch (const std::runtime_error & err) {
        LOG_ERROR ("ComposeService::submitNewPost : ", __FILE__, " ", __LINE__, " ", err.what ());
        return response (ResponseCode::MALFORMED);
      }


      auto postService = socialNet::findService (this-> _system, this-> _registry, "post");
      auto postReq = config::Dict ()
        .insert ("type", RequestCode::STORE)
        .insert ("userLogin", (*userResult) ["content"].getStr ())
        .insert ("userId", msg ["userId"].getI ())
        .insert ("text", (*textResult) ["content"]["text"].getStr ())
        .insert ("jwt_token", msg ["jwt_token"].getStr ())
        .insert ("tags", arr);

      try {
        auto result = postService-> request (postReq).wait ();
        if (result == nullptr || result-> getOr ("code", -1) != ResponseCode::OK) {
          throw std::runtime_error ("Failed to store");
        }
        return response (ResponseCode::OK);
      } catch (const std::runtime_error & err) {
        LOG_ERROR ("ComposeService::submitNewPost : ", __FILE__, " ", __LINE__, " ", err.what ());
        return response (ResponseCode::MALFORMED);
      }
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("ComposeService::submitNewPost : ", __FILE__, " ", __LINE__, " ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ================================          TIMELINE STREAM          =================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <config::ConfigNode> ComposeService::readTimeline (const config::ConfigNode & msg, RequestCode kind) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      uint32_t page = msg ["page"].getI ();
      uint32_t nb = msg ["nb"].getI ();

      auto timeline = this-> retreiveTimeline (uid, page, nb, kind);
      if (timeline-> getOr ("code", -1) == ResponseCode::OK) {
        match ((*timeline)["content"]) {
          of (config::Array, arr) {
            auto postService = socialNet::findService (this-> _system, this-> _registry, "post");
            auto result = std::make_shared <config::Array> ();
            for (uint32_t i = 0 ; i < arr-> getLen () ; i++) {
              uint32_t id = (*arr) [i].getI ();
              auto req = config::Dict ()
                .insert ("type", RequestCode::READ_POST)
                .insert ("postId", std::make_shared <config::Int> (id));

              auto post = postService-> request (req).wait ();
              if (post && post-> getOr ("code", -1) == ResponseCode::OK) {
                result-> insert (post-> get ("content"));
              }
            }

            return response (ResponseCode::OK, result);
          }

          elfo {
            return response (ResponseCode::SERVER_ERROR);
          }
        }
      } else return timeline;
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("Error ComposeService::streamTimeline ", __FILE__, __LINE__, " ", err.what ());
    }

    return response (ResponseCode::MALFORMED);
  }

   std::shared_ptr <config::ConfigNode> ComposeService::retreiveTimeline (uint32_t uid, uint32_t page, uint32_t nb, RequestCode kind) {
    try {
      auto timelineService = socialNet::findService (this-> _system, this-> _registry, "timeline");
      auto timelineReq = config::Dict ()
        .insert ("type", kind)
        .insert ("userId", (int64_t) uid)
        .insert ("page", (int64_t) page)
        .insert ("nb", (int64_t) nb);

      auto ret = timelineService-> request (timelineReq).wait ();
      if (ret == nullptr) return response (ResponseCode::SERVER_ERROR);

      return ret;
    } catch (std::runtime_error & err) {
      LOG_ERROR ("ComposeService::openTimelineStreams : ", __FILE__, __LINE__, " ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          SUBS STREAM          ===================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <config::ConfigNode> ComposeService::readSubscriptions (const config::ConfigNode & msg, RequestCode kind) {
try {
      uint32_t uid = msg ["userId"].getI ();
      uint32_t page = msg ["page"].getI ();
      uint32_t nb = msg ["nb"].getI ();

      auto subs = this-> retreiveFollSubs (uid, page, nb, kind);
      if (subs-> getOr ("code", -1) == ResponseCode::OK) {
        match ((*subs)["content"]) {
          of (config::Array, arr) {
            auto userService = socialNet::findService (this-> _system, this-> _registry, "user");

            auto result = std::make_shared <config::Array> ();
            for (uint32_t i = 0 ; i < arr-> getLen () ; i++) {
              uint32_t id = (*arr) [i].getI ();
              auto req = config::Dict ()
                .insert ("type", RequestCode::FIND)
                .insert ("id", std::make_shared <config::Int> (id));

              auto user = userService-> request (req).wait ();
              if (user && user-> getOr ("code", -1) == ResponseCode::OK) {
                auto u = std::make_shared <config::Dict> ();
                u-> insert ("login", user-> get ("content"));
                u-> insert ("id", std::make_shared <config::Int> (id));

                result-> insert (u);
              }
            }

            return response (ResponseCode::OK, result);
          }

          elfo {
            return response (ResponseCode::SERVER_ERROR);
          }
        }
      } else return subs;
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("Error ComposeService::streamTimeline ", __FILE__, __LINE__, " ", err.what ());
    }

    return response (ResponseCode::MALFORMED);
  }

  std::shared_ptr <config::ConfigNode> ComposeService::retreiveFollSubs (uint32_t uid, uint32_t page, uint32_t nb, RequestCode kind) {
    try {
      auto socialService = socialNet::findService (this-> _system, this-> _registry, "social_graph");
      auto socialReq = config::Dict ()
        .insert ("type", kind)
        .insert ("userId", (int64_t) uid)
        .insert ("page", (int64_t) page)
        .insert ("nb", (int64_t) nb);

      auto ret = socialService-> request (socialReq).wait ();
      if (ret == nullptr) return response (ResponseCode::SERVER_ERROR);

      return ret;
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("ComposeService::openSubStreams : ", __FILE__, ":", __LINE__, " ", err.what ());

      return response (ResponseCode::MALFORMED);
    }
  }

}
