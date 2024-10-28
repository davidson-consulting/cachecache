#define LOG_LEVEL 10
#define __PROJECT__ "TIMELINE"

#include "../../utils/jwt/check.hh"
#include "service.hh"
#include "../../registry/service.hh"

using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet::timeline {

  TimelineService::TimelineService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf) :
    actor::ActorBase (name, sys)
  {
    this-> _db.configure (conf);

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    this-> _secret = conf ["auth"]["secret"].getStr ();
    this-> _issuer = conf ["auth"].getOr ("issuer", "auth0");

    socialNet::registerService (this-> _registry, "timeline", name, sys-> port (), this-> _iface);
  }

  void TimelineService::onQuit () {
    socialNet::closeService (this-> _registry, "timeline", this-> _name, this-> _system-> port (), this-> _iface);
  }


  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> TimelineService::onRequest (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", "none");
    if (type == "count-posts") {
        return this-> countPosts (msg);
    }

    if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
      if (type == "update") {
        return this-> updatePosts (msg);
      } else if (type == "count-home") {
        return this-> countHome (msg);
      }
      return ResponseCode (404);
    } else {
      return ResponseCode (403);
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> TimelineService::updatePosts (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      uint32_t pid = msg ["postId"].getI ();
      this-> _db.insertOneHome (uid, pid);
      this-> _db.insertOnePost (uid, pid);

      this-> updateForFollowers (uid, pid);
      match (msg ["tags"]) {
        of (config::Array, a) {
          for (uint32_t i = 0 ; i < a-> getLen () ; i++) {
            this-> _db.insertOneHome ((*a)[i].getI (), pid);
          }
        } fo;
      }


      return ResponseCode (200);
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    return ResponseCode (400);
  }

  void TimelineService::updateForFollowers (uint32_t uid, uint32_t pid) {
    auto socialService = socialNet::findService (this-> _system, this-> _registry, "social_graph");
    if (socialService == nullptr) {
      LOG_ERROR ("No social graph service found");
    }

    auto req = config::Dict ()
      .insert ("type", std::make_shared <config::String> ("followers"))
      .insert ("userId", std::make_shared <config::Int> (uid));

    uint32_t follower = 0;

    auto followStreamReq = socialService-> requestStream (req);

    auto prep = this-> _db.prepareInsertHomeTimeline (&follower, pid);
    auto followStream = followStreamReq.wait ();
    if (followStream == nullptr) { LOG_ERROR ("Failed to connect to stream"); }
    else {
      while (true) {
        auto has = followStream-> readU8 ();
        if (has == 1) {
          follower = followStream-> readU32 ();
          prep-> execute ();
        } else break;
      }
    }
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          FINDING         ========================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<rd_utils::utils::config::ConfigNode> TimelineService::countPosts (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      auto nb = this-> _db.countPosts (uid);

      return ResponseCode (200, std::make_shared <config::Int> (nb));
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    return ResponseCode (400);
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> TimelineService::countHome (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      auto nb = this-> _db.countHome (uid);

      return ResponseCode (200, std::make_shared <config::Int> (nb));
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

  void TimelineService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    auto type = msg.getOr ("type", "none");
    if (type == "home") {
      this-> streamHome (msg, stream);
    } else if (type == "posts") {
      this-> streamPosts (msg, stream);
    }
  }

  void TimelineService::streamHome (const config::ConfigNode & msg, actor::ActorStream & stream) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      int32_t nb = msg.getOr ("nb", -1);
      int32_t page = msg.getOr ("page", -1) * nb;

      uint32_t pid;
      auto statement = this-> _db.prepareFindHome (&pid, &uid, &page, &nb);
      statement-> execute ();
      while (statement-> next () && stream.isOpen ()) {
        stream.writeU8 (1);
        stream.writeU32 (pid);
      }
    } catch (std::runtime_error & err) {
      LOG_INFO ("ERROR : ", err.what ());
    } catch (...) {
    }

    if (stream.isOpen ()) {
      stream.writeU8 (0);
    }
  }

  void TimelineService::streamPosts (const config::ConfigNode & msg, actor::ActorStream & stream) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      int32_t nb = msg.getOr ("nb", -1);
      int32_t page = msg.getOr ("page", -1) * nb;

      uint32_t pid;
      auto statement = this-> _db.prepareFindPosts (&pid, &uid, &page, &nb);
      statement-> execute ();
      while (statement-> next () && stream.isOpen ()) {
        stream.writeU8 (1);
        stream.writeU32 (pid);
      }

      stream.writeU8 (0);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("ERROR : ", err.what ());
    }

    if (stream.isOpen ()) {
      stream.writeU8 (0);
    }
  }


}
