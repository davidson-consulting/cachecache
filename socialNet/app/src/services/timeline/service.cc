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
    CONF_LET (dbName, conf["services"]["timeline"]["db"].getStr (), std::string ("mysql"));
    CONF_LET (chName, conf["services"]["timeline"]["cache"].getStr (), std::string (""));
    // this-> _insertDb.configure (dbName, chName, conf);
    // this-> _req100 = this-> _insertDb.prepareBuffered (MAX_NB_INSERT);
    // this-> _req10 = this-> _insertDb.prepareBuffered (10);
    // this-> _readDb.configure (dbName, chName, conf);

    this-> _db.configure (chName, conf);

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    this-> _secret = conf ["auth"]["secret"].getStr ();
    this-> _issuer = conf ["auth"].getOr ("issuer", "auth0");
    this-> _freq = conf ["services"]["timeline"].getOr ("freq", 1.0f);
  }

  void TimelineService::onStart () {
    this-> _running  = true;
    this-> _routine = concurrency::spawn (this, &TimelineService::treatRoutine);

    socialNet::registerService (this-> _registry, "timeline", this-> _name, this-> _system-> port (), this-> _iface);
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
      this-> _registry = nullptr;
    }

    // this-> _req10.reset ();
    // this-> _req100.reset ();
    // this-> _insertDb.dispose ();
    // this-> _readDb.dispose ();

    this-> _db.dispose ();
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

      elof_v (RequestCode::HOME_TIMELINE) {
        return this-> readHome (msg);
      }

      elof_v (RequestCode::USER_TIMELINE) {
        return this-> readPost (msg);
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
   * ====================================          READING          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<rd_utils::utils::config::ConfigNode> TimelineService::readHome (const config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      int32_t nb = msg ["nb"].getI ();
      int32_t page = msg ["page"].getI ();

      auto result = std::make_shared <config::Array> ();
      auto values = this-> _db.findHomeCacheable (uid, page, nb);
      for (auto & it : values) {
        result-> insert (std::make_shared <config::Int> (it));
      }

      return response (ResponseCode::OK, result);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TimelineService::streamHome : ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> TimelineService::readPost (const config::ConfigNode & msg) {
    try {
      uint32_t uid = msg ["userId"].getI ();
      int32_t nb = msg ["nb"].getI ();
      int32_t page = msg ["page"].getI ();

      auto result = std::make_shared <config::Array> ();
      auto values = this-> _db.findPostCacheable (uid, page, nb);
      for (auto & it : values) {
        result-> insert (std::make_shared <config::Int> (it));
      }

      return response (ResponseCode::OK, result);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TimelineService::streamHome : ", err.what ());
      return response (ResponseCode::MALFORMED);
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
    std::map <uint32_t, std::vector <PostUpdate> > cp;
    while (this-> _running) {
      t.reset ();
      LOG_DEBUG ("Timeline iteration");
      LOG_DEBUG ("Timeline wait lock");

      try {
        WITH_LOCK (this-> _m) { // instances cannot register/quit during a market run
          cp = std::move (this-> _toUpdates);
        }

        for (auto & it : cp) {
          LOG_INFO ("Starting update : ", it.first);
          this-> updateForFollowers (it.first, it.second);
        }

        this-> _db.commit ();
        LOG_INFO ("Iteration took : ", t.time_since_start ());
      } catch (const std::runtime_error & err) {
        LOG_ERROR ("Error in timeline update : ", err.what ());
      }

      auto took = t.time_since_start ();
      auto suppose = 1.0f / this-> _freq;
      if (took < suppose) {
        LOG_DEBUG ("Sleep ", suppose - took);
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

    for (auto up : updates) {
      this-> _db.insertHome (uid, up.pid);
      this-> _db.insertPost (uid, up.pid);
      for (auto taggedId : up.tagged) {
        this-> _db.insertHome (taggedId, up.pid);
      }
    }

    for (uint32_t page = 0 ;; page++) {
      auto msg = config::Dict ()
        .insert ("type", RequestCode::FOLLOWERS)
        .insert ("uid", std::make_shared <config::Int> (uid))
        .insert ("page", std::make_shared <config::Int> (page))
        .insert ("nb", std::make_shared <config::Int> (50));

      auto resp = socialService-> request (msg).wait ();
      if (resp && resp-> getOr ("code", -1) == 200) {
        match ((*resp) ["content"]) {
          of (config::Array, arr) {
            for (uint32_t i = 0 ; i < arr-> getLen () ; i++) {
              CONF_LET (follower, (*arr) [i].getI (), 0);
              for (auto & up : updates) {
                if (up.tagged.find (follower) == up.tagged.end ()) {
                  this-> _db.insertHome (follower, up.pid);
                }
              }
            }

            if (arr-> getLen () < 50) break;
          } fo;
        }
      } else {
        LOG_ERROR ("Failed updating timelines");
        break;
      }
    }
  }


}
