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
      if (userService == nullptr) return response (ResponseCode::SERVER_ERROR);

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
      if (userService == nullptr) return response (ResponseCode::SERVER_ERROR);

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
      auto postService = socialNet::findService (this-> _system, this-> _registry, "post");
      auto userService = socialNet::findService (this-> _system, this-> _registry, "user");

      if (textService == nullptr || postService == nullptr || userService == nullptr) {
        return response (ResponseCode::SERVER_ERROR);
      }

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

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          STREAMING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */


  void ComposeService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::HOME_TIMELINE) {
        this-> streamTimeline (msg, stream, RequestCode::HOME_TIMELINE);
      }

      elof_v (RequestCode::USER_TIMELINE) {
        this-> streamTimeline (msg, stream, RequestCode::USER_TIMELINE);
      }

      elof_v (RequestCode::SUBSCIRPTIONS) {
        this-> streamSubscriptions (msg, stream, RequestCode::SUBSCIRPTIONS);
      }

      elof_v (RequestCode::FOLLOWERS) {
        this-> streamSubscriptions (msg, stream, RequestCode::FOLLOWERS);
      }

      elfo {
        stream.writeU32 (ResponseCode::NOT_FOUND);
      }
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ================================          TIMELINE STREAM          =================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ComposeService::streamTimeline (const config::ConfigNode & msg, actor::ActorStream & stream, RequestCode kind) {
      std::shared_ptr <actor::ActorStream> timelineStream;
      std::shared_ptr <actor::ActorStream> postStream;

      if (this-> openTimelineStreams (msg, kind, stream, timelineStream, postStream)) {
        try {
          socialNet::post::Post p;

          while (timelineStream-> readOr ((uint8_t) 0) != 0) {
            auto pid = timelineStream-> readU32 ();
            postStream-> writeU8 (1);
            postStream-> writeU32 (pid);
            if (postStream-> readOr ((uint8_t) 0) != 0) {;
              postStream-> readRaw (p);
              stream.writeU8 (1);
              stream.writeRaw (p);
            }
          }

          postStream-> writeU8 (0);
          stream.writeU8 (0);
        }  catch (const std::runtime_error & err) {
          LOG_ERROR ("Error ComposeService::streamTimeline ", __FILE__, __LINE__, " ", err.what (), " ", timelineStream-> isOpen (), " ", postStream-> isOpen (), " ", stream.isOpen ());
          if (postStream-> isOpen ()) {
            postStream-> writeU8 (0);
          }
          stream.writeU8 (0);
        }
      }
  }

  bool ComposeService::openTimelineStreams (const config::ConfigNode & msg, RequestCode kind, actor::ActorStream & stream, std::shared_ptr <actor::ActorStream> & timelineStream, std::shared_ptr <actor::ActorStream> & postStream) {
    try {
      auto postService = socialNet::findService (this-> _system, this-> _registry, "post");
      auto timelineService = socialNet::findService (this-> _system, this-> _registry, "timeline");

      if (postService == nullptr || timelineService == nullptr) {
        stream.writeU32 (ResponseCode::SERVER_ERROR);
        return false;
      }

      uint32_t uid = msg ["userId"].getI ();
      uint32_t page = msg.getOr ("page", -1);
      uint32_t nb = msg.getOr ("nb", -1);

      auto timelineReq = config::Dict ()
        .insert ("type", kind)
        .insert ("userId", (int64_t) uid)
        .insert ("page", (int64_t) page)
        .insert ("nb", (int64_t) nb);

      auto postReq = config::Dict ()
        .insert ("type", RequestCode::READ_POST);

      auto timelineStreamReq = timelineService-> requestStream (timelineReq);
      auto postStreamReq = postService-> requestStream (postReq);

      timelineStream = timelineStreamReq.wait ();
      postStream = postStreamReq.wait ();

      if (timelineStream == nullptr || postStream == nullptr) {
        stream.writeU32 (ResponseCode::SERVER_ERROR);
        return false;
      }

      auto codeA = timelineStream-> readU32 ();
      auto codeB = postStream-> readU32 ();

      if (codeA != ResponseCode::OK || codeB != ResponseCode::OK) {
        stream.writeU32 (ResponseCode::SERVER_ERROR);
        return false;
      }

      stream.writeU32 (ResponseCode::OK);
      return true;
    } catch (std::runtime_error & err) {
      LOG_ERROR ("ComposeService::openTimelineStreams : ", __FILE__, __LINE__, " ", err.what ());

      return false;
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          SUBS STREAM          ===================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ComposeService::streamSubscriptions (const config::ConfigNode & msg, actor::ActorStream & stream, RequestCode kind) {
    std::shared_ptr <actor::ActorStream> socialStream, userStream;
    if (this-> openSubStreams (msg, kind, stream, socialStream, userStream)) {
      try {

        while (socialStream-> readOr (0) != 0) {
          auto rid = socialStream-> readU32 ();
          userStream-> writeU8 (1);
          userStream-> writeU32 (rid);

          if (userStream-> readOr (0) != 0) {
            auto name = userStream-> readStr ();
            stream.writeU8 (1);
            stream.writeU32 (rid);
            stream.writeStr (name);
          }
        }

        userStream-> writeU8 (0);
        stream.writeU8 (0);
      } catch (const std::runtime_error & err) {
        LOG_ERROR ("Error ComposeService::streamSubs ", __FILE__, __LINE__, " ", err.what ());
      }
    }
  }

  bool ComposeService::openSubStreams (const config::ConfigNode & msg, RequestCode kind, actor::ActorStream & stream, std::shared_ptr <actor::ActorStream> & socialStream, std::shared_ptr <actor::ActorStream> & userStream) {
    try {
      auto socialService = socialNet::findService (this-> _system, this-> _registry, "social_graph");
      auto userService = socialNet::findService (this-> _system, this-> _registry, "user");

      if (userService == nullptr || socialService == nullptr) {
        stream.writeU32 (ResponseCode::SERVER_ERROR);
        return false;
      }

      uint32_t uid = msg ["userId"].getI ();
      uint32_t page = msg.getOr ("page", -1);
      uint32_t nb = msg.getOr ("nb", -1);

      auto socialReq = config::Dict ()
        .insert ("type", kind)
        .insert ("userId", (int64_t) uid)
        .insert ("page", (int64_t) page)
        .insert ("nb", (int64_t) nb);

      auto userReq = config::Dict ()
        .insert ("type", RequestCode::FIND_BY_ID);

      auto socialStreamReq = socialService-> requestStream (socialReq);
      auto userStreamReq = userService-> requestStream (userReq);

      userStream = userStreamReq.wait ();
      socialStream = socialStreamReq.wait ();

      if (userStream == nullptr || socialStream == nullptr) {
        stream.writeU32 (ResponseCode::SERVER_ERROR);
        return false;
      }

      auto codeB = socialStream-> readU32 ();
      auto codeA = userStream-> readU32 ();

      if (codeA != ResponseCode::OK || codeB != ResponseCode::OK) {
        stream.writeU32 (ResponseCode::SERVER_ERROR);
        return false;
      }

      stream.writeU32 (ResponseCode::OK);
      return true;
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("ComposeService::openSubStreams : ", __FILE__, ":", __LINE__, " ", err.what ());

      return false;
    }
  }

}
