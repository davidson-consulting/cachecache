#define __PROJECT__ "TIMELINE"

#include "../../utils/jwt/check.hh"
#include "service.hh"
#include "../../registry/service.hh"

using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;
using namespace rd_utils;

using namespace socialNet::utils;

#define MAX_NB_INSERT 100

namespace socialNet::timeline {

  TimelineService::TimelineService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf)
    : actor::ActorBase (name, sys)
    , _routine (0, nullptr)
    , _running (false)
  {
    this-> _db.configure (conf);
    this-> _req100 = this-> _db.prepareBuffered (MAX_NB_INSERT);
    this-> _req10 = this-> _db.prepareBuffered (10);

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    this-> _secret = conf ["auth"]["secret"].getStr ();
    this-> _issuer = conf ["auth"].getOr ("issuer", "auth0");
    this-> _freq = conf ["services"]["timeline"].getOr ("freq", 1.0f);

    socialNet::registerService (this-> _registry, "timeline", name, sys-> port (), this-> _iface);
  }

  void TimelineService::onStart () {
    this-> _running  = true;
    this-> _routine = concurrency::spawn (this, &TimelineService::treatRoutine);
  }

  void TimelineService::onQuit () {
    if (this-> _routine.id != 0) {
      this-> _running = false;
      this-> _routineSleeping.post ();

      concurrency::join (this-> _routine);
      this-> _routine = concurrency::Thread (0, nullptr);
    }

    if (this-> _registry != nullptr) {
      socialNet::closeService (this-> _registry, "timeline", this-> _name, this-> _system-> port (), this-> _iface);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          UPDATING          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void TimelineService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _registry = nullptr;
        this-> exit ();
      }

      elof_v (RequestCode::UPDATE_TIMELINE) {
        if (utils::checkConnected (msg, this-> _issuer, this-> _secret)) {
          this-> updatePosts (msg);
        }
      }

      fo;
    }
  }


  void TimelineService::updatePosts (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      uint32_t pid = msg ["postId"].getI ();
      std::set <uint32_t> tagged;
      match (msg ["tags"]) {
        of (config::Array, a) {
          for (uint32_t i = 0 ; i < a-> getLen () ; i++) {
            auto taggedId = (*a)[i].getI ();
            if (taggedId != uid) {
              tagged.emplace (taggedId);
            }
          }
        } fo;
      }


      WITH_LOCK (this-> _m) {
        auto it = this-> _toUpdates.find (uid);
        if (it != this-> _toUpdates.end ()) {
          it-> second.push_back (PostUpdate {.pid = pid, .tagged = tagged});
        } else {
          std::vector <PostUpdate> up;
          up.push_back (PostUpdate {.pid = pid, .tagged = tagged});
          this-> _toUpdates.emplace (uid, up);
        }
      }
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TimelineService::updatePosts : ", err.what ());
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

      elfo {
        return response (ResponseCode::NOT_FOUND);
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


  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          BATCH UPDATE          ==================================
   * ====================================================================================================
   * ====================================================================================================
   */


  void TimelineService::treatRoutine (rd_utils::concurrency::Thread) {
    concurrency::timer t;
    while (this-> _running) {
      t.reset ();
      LOG_INFO ("Timeline iteration");
      LOG_DEBUG ("Timeline wait lock");

      try {
        std::map <uint32_t, std::vector <PostUpdate> > cp;
        WITH_LOCK (this-> _m) { // instances cannot register/quit during a market run
          cp = std::move (this-> _toUpdates);
        }

        for (auto & it : cp) {
          this-> updateForFollowers (it.first, it.second);
        }
      } catch (const std::runtime_error & err) {
        LOG_ERROR ("Error in timeline update : ", err.what ());
      }

      auto took = t.time_since_start ();
      auto suppose = 1.0f / this-> _freq;
      if (took < suppose) {
        LOG_INFO ("Sleep ", suppose - took);
        if (this-> _routineSleeping.wait (suppose - took)) {
          LOG_INFO ("Interupted");
          return;
        }
      }
    }
  }

  void TimelineService::updateForFollowers (uint32_t uid, const std::vector <TimelineService::PostUpdate> & updates) {
    auto socialService = socialNet::findService (this-> _system, this-> _registry, "social_graph");
    if (socialService == nullptr) {
      throw std::runtime_error ("No social graph service found");
    }

    auto req = config::Dict ()
      .insert ("type", RequestCode::FOLLOWERS)
      .insert ("userId", (int64_t) uid);
    auto followStreamReq = socialService-> requestStream (req);

    for (auto up : updates) {
      this-> _db.insertOneHome (uid, up.pid);
      this-> _db.insertOnePost (uid, up.pid);
      for (auto taggedId : up.tagged) {
        this-> _db.insertOneHome (taggedId, up.pid);
      }
    }

    auto followStream = followStreamReq.wait ();
    if (followStream == nullptr || followStream-> readU32 () != ResponseCode::OK) {
      throw std::runtime_error ("Failed to connect to stream");
    }

    uint32_t pids [MAX_NB_INSERT];
    uint32_t followers [MAX_NB_INSERT];
    uint32_t nb = 0;

    while (followStream-> readOr (0) == 1) {
      auto follower = followStream-> readU32 ();
      for (auto & up : updates) {
        if (up.tagged.find (follower) == up.tagged.end ()) {
          followers [nb] = follower;
          pids [nb] = up.pid;
          nb += 1;

          if (nb == MAX_NB_INSERT) { // insert 100 per 100
            this-> _db.executeBuffered (this-> _req100, followers, pids, MAX_NB_INSERT);
            nb = 0;
          }
        }
      }
    }

    uint32_t i = 0;
    for (; nb >= 10 ; i += 10) {
      this-> _db.executeBuffered (this-> _req10, &followers[i], &pids[i], 10);
      nb -= 10;
    }

    for (; nb > 0 ; i++) {
      this-> _db.insertOneHome (followers [i], pids [i]);
      nb -= 1;
    }

    this-> _db.commit ();
  }


}
