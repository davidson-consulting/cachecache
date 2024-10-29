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

  void TimelineService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _registry = nullptr;
        this-> exit ();
      }

      fo;
    }
  }

  void TimelineService::onQuit () {
    if (this-> _registry != nullptr) {
      socialNet::closeService (this-> _registry, "timeline", this-> _name, this-> _system-> port (), this-> _iface);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          REQUESTS          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> TimelineService::onRequest (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::HOME_TIMELINE_LEN) {
        return this-> countPosts (msg);
      }

      elof_v (RequestCode::USER_TIMELINE_LEN) {
        return this-> countHome (msg);
      }

      elof_v (RequestCode::UPDATE_TIMELINE) {
        if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
          return this-> updatePosts (msg);
        } else {
          return response (ResponseCode::FORBIDDEN);
        }
      }

      elfo {
        return response (ResponseCode::NOT_FOUND);
      }
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> TimelineService::updatePosts (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      uint32_t pid = msg ["postId"].getI ();
      this-> _db.insertOneHome (uid, pid);
      this-> _db.insertOnePost (uid, pid);

      std::set <int64_t> tagged;
      match (msg ["tags"]) {
        of (config::Array, a) {
          for (uint32_t i = 0 ; i < a-> getLen () ; i++) {
            auto taggedId = (*a)[i].getI ();
            if (taggedId != uid) {
              this-> _db.insertOneHome (taggedId, pid);
              tagged.emplace (taggedId);
            }
          }
        } fo;
      }

      this-> updateForFollowers (uid, pid, tagged);

      return response (ResponseCode::OK);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TimelineService::updatePosts : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  void TimelineService::updateForFollowers (uint32_t uid, uint32_t pid, const std::set <int64_t> & tagged) {
    auto socialService = socialNet::findService (this-> _system, this-> _registry, "social_graph");
    if (socialService == nullptr) {
      throw std::runtime_error ("No social graph service found");
    }

    auto req = config::Dict ()
      .insert ("type", RequestCode::FOLLOWERS)
      .insert ("userId", (int64_t) uid);

    uint32_t follower = 0;
    auto followStreamReq = socialService-> requestStream (req);

    auto prep = this-> _db.prepareInsertHomeTimeline (&follower, &pid);
    auto followStream = followStreamReq.wait ();
    if (followStream == nullptr || followStream-> readU32 () != ResponseCode::OK) {
      throw std::runtime_error ("Failed to connect to stream");
    }

    while (followStream-> readOr (0) == 1) {
      follower = followStream-> readU32 ();
      if (tagged.find (follower) == tagged.end ()) {
        prep-> execute ();
      }
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          FINDING          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<rd_utils::utils::config::ConfigNode> TimelineService::countPosts (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      auto nb = this-> _db.countPosts (uid);

      return response (ResponseCode::OK, std::make_shared <config::Int> (nb));
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TimelineService::countPosts : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> TimelineService::countHome (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      auto nb = this-> _db.countHome (uid);

      return response (ResponseCode::OK, std::make_shared <config::Int> (nb));
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TimelineService::countHome : ", err.what ());
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

  void TimelineService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::HOME_TIMELINE) {
        this-> streamHome (msg, stream);
      }

      elof_v (RequestCode::USER_TIMELINE) {
        this-> streamPosts (msg, stream);
      }

      elfo {
        stream.writeU32 (ResponseCode::NOT_FOUND);
      }
    }
  }

  void TimelineService::streamHome (const config::ConfigNode & msg, actor::ActorStream & stream) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      int32_t nb = msg.getOr ("nb", -1);
      int32_t page = msg.getOr ("page", -1);
      if (page >= 0) page *= nb;

      uint32_t pid;
      auto statement = this-> _db.prepareFindHome (&pid, &uid, &page, &nb);
      statement-> execute ();

      stream.writeU32 (ResponseCode::OK);
      while (statement-> next () && stream.isOpen ()) {
        stream.writeU8 (1);
        stream.writeU32 (pid);
      }

      stream.writeU8 (0);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TimelineService::streamHome : ", err.what ());
    }
  }

  void TimelineService::streamPosts (const config::ConfigNode & msg, actor::ActorStream & stream) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      int32_t nb = msg.getOr ("nb", -1);
      int32_t page = msg.getOr ("page", -1);
      if (page >= 0) page *= nb;

      uint32_t pid;
      auto statement = this-> _db.prepareFindPosts (&pid, &uid, &page, &nb);
      statement-> execute ();

      stream.writeU32 (ResponseCode::OK);
      while (statement-> next () && stream.isOpen ()) {
        stream.writeU8 (1);
        stream.writeU32 (pid);
      }

      stream.writeU8 (0);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TimelineService::streamPosts : ", err.what ());
    }
  }


}
