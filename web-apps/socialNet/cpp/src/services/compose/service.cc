#define LOG_LEVEL 10
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

    socialNet::registerService (this-> _registry, "compose", name, sys-> port (), this-> _iface);
  }

  void ComposeService::onQuit () {
    socialNet::closeService (this-> _registry, "compose", this-> _name, this-> _system-> port (), this-> _iface);
  }


  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */


  std::shared_ptr<config::ConfigNode> ComposeService::onRequest (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", "none");
    if (type == "submit") {
      if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
        return this-> submitNewPost (msg);
      } else return ResponseCode (403);
    } else if (type == "register") {
      return this-> registerUser (msg);
    } else if (type == "login") {
      return this-> login (msg);
    } else if (type == "check") {
      if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
        return ResponseCode (200);
      } else {
        return ResponseCode (403);
      }
    }

    return nullptr;
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
      if (userService == nullptr) return ResponseCode (500);

      config::Dict userReq = config::Dict ()
        .insert ("type", std::string ("login"))
        .insert ("login", msg ["login"].getStr ())
        .insert ("password", msg ["password"].getStr ());

      auto result = userService-> request (userReq).wait ();
      if (result && result-> getOr ("code", -1) == 200) {
        auto resp = config::Dict ()
          .insert ("userId", (*result) ["content"].getI ())
          .insert ("jwt_token", utils::createJWTToken ((*result) ["content"].getI (), this-> _issuer, this-> _secret));

        return ResponseCode (200, resp);
      } else {
        LOG_ERROR ("No response, or wrong code ?");
        return ResponseCode (403);
      }
    } catch (std::runtime_error & err) {
      LOG_ERROR ("Compose::login, Error : ", err.what ());
    }

    return ResponseCode (400);
  }

  std::shared_ptr<config::ConfigNode> ComposeService::registerUser (const config::ConfigNode & msg) {
    try {
      auto userService = socialNet::findService (this-> _system, this-> _registry, "user");
      if (userService == nullptr) return ResponseCode (500);

      auto userReq = config::Dict ()
        .insert ("type", "register")
        .insert ("login", msg ["login"].getStr ())
        .insert ("password", msg ["password"].getStr ());

      auto result = userService-> request (userReq).wait ();
      if (result && result-> getOr ("code", -1) == 200) {
        return ResponseCode (200);
      } else {
        return ResponseCode (403);
      }
    } catch (...) {
    }

    return ResponseCode (400);
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
        return ResponseCode (500);
      }

      auto textReq = config::Dict ()
        .insert ("type", "ctor")
        .insert ("text", msg ["content"].getStr ());

      auto userReq = config::Dict ()
        .insert ("type", "find")
        .insert ("id", msg ["userId"].getI ());

      auto textResReq = textService-> request (textReq);
      auto userResReq = userService-> request (userReq);

      auto textResult = textResReq.wait ();

      if (textResult == nullptr || textResult-> getOr ("code", -1) != 200) throw std::runtime_error ("Text incorrect");

      auto arr = std::make_shared <config::Array> ();
      match ((*textResult) ["content"]["tags"]) {
        of (config::Array, a) {
          for (uint32_t i = 0 ; i < a-> getLen () && i < 16 ; i++) {
            arr-> insert ((*a) [i].getI ());
          }
        } fo;
      }

      auto userResult = userResReq.wait ();
      if (userResult == nullptr || userResult-> getOr ("code", -1) != 200) throw std::runtime_error ("User not found");

      auto postReq = config::Dict ()
        .insert ("type", "store")
        .insert ("userLogin", (*userResult) ["content"].getStr ())
        .insert ("userId", msg ["userId"].getI ())
        .insert ("text", (*textResult) ["content"]["text"].getStr ())
        .insert ("jwt_token", msg ["jwt_token"].getStr ())
        .insert ("tags", arr);

      auto result = postService-> request (postReq).wait ();
      if (result == nullptr || result-> getOr ("code", -1) != 200) throw std::runtime_error ("Failed to store");

      return ResponseCode (200);
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR ComposeService::submitNewPost : ", err.what ());
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


  void ComposeService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    auto type = msg.getOr ("type", "none");
    if (type == "home_t") {
      if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
        this-> streamTimeline (msg, stream, "home");
      } else stream.close ();
    } else if (type == "user_t") {
      this-> streamTimeline (msg, stream, "posts");
    } else if (type == "subs") {
      if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
        this-> streamSubscriptions (msg, stream, "subscriptions");
      } else stream.close ();
    } else if (type == "followers") {
      this-> streamSubscriptions (msg, stream, "followers");
    } else {
      stream.close ();
    }


  }

  void ComposeService::streamTimeline (const config::ConfigNode & msg, actor::ActorStream & stream, const std::string & kind) {
    try {
      auto postService = socialNet::findService (this-> _system, this-> _registry, "post");
      auto timelineService = socialNet::findService (this-> _system, this-> _registry, "timeline");

      if (postService == nullptr || timelineService == nullptr) {
        throw std::runtime_error ("failed to find adequate micro service");
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
        .insert ("type", "posts");

      auto timelineStreamReq = timelineService-> requestStream (timelineReq);
      auto postStreamReq = postService-> requestStream (postReq);

      auto timelineStream = timelineStreamReq.wait ();
      auto postStream = postStreamReq.wait ();

      if (timelineStream == nullptr) throw std::runtime_error ("failed to open stream with timeline service");
      if (postStream == nullptr) throw std::runtime_error ("failed to open stream with post service");

      socialNet::post::Post p;

      while (timelineStream-> isOpen () && postStream-> isOpen ()) {
        if (timelineStream-> readOr ((uint8_t) 0) != 0) {
          auto pid = timelineStream-> readU32 ();
          postStream-> writeU8 (1);
          postStream-> writeU32 (pid);
          if (postStream-> readOr ((uint8_t) 0) != 0) {;
            postStream-> readRaw (p);
            stream.writeU8 (1);
            stream.writeRaw (p);
          }
        } else {
          postStream-> writeU8 (0);
          break;
        }
      }

    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    if (stream.isOpen ()) {
      stream.writeU8 (0);
    }
  }

  void ComposeService::streamSubscriptions (const config::ConfigNode & msg, actor::ActorStream & stream, const std::string & kind) {
    try {
      auto socialService = socialNet::findService (this-> _system, this-> _registry, "social_graph");
      auto userService = socialNet::findService (this-> _system, this-> _registry, "user");

      if (userService == nullptr || socialService == nullptr) {
        return ;
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
        .insert ("type", "find_by_id");

      auto socialStreamReq = socialService-> requestStream (socialReq);
      auto userStreamReq = userService-> requestStream (userReq);

      auto userStream = userStreamReq.wait ();
      auto socialStream = socialStreamReq.wait ();

      if (userStream == nullptr) throw std::runtime_error ("failed to access user stream");
      if (socialStream == nullptr) throw std::runtime_error ("failed to access social stream");

      while (socialStream-> isOpen () && userStream-> isOpen ()) {
        auto next = socialStream-> readU8 ();
        if (next != 0 && socialStream-> isOpen ()) {
          auto rid = socialStream-> readU32 ();
          userStream-> writeU8 (1);
          userStream-> writeU32 (rid);

          if (userStream-> readOr (0) != 0) {
            auto name = userStream-> readStr ();
            stream.writeU8 (1);
            stream.writeU32 (rid);
            stream.writeStr (name);
          }
        } else {
          userStream-> writeU8 (0);
          break;
        }
      }
    }  catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    if (stream.isOpen ()) {
      stream.writeU8 (0);
    }
  }

}
