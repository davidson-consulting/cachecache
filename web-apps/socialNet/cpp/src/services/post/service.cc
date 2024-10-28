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
    this-> _db.configure (conf);

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    socialNet::registerService (this-> _registry, "post", name, sys-> port (), this-> _iface);
  }


  void PostStorageService::onQuit () {
    socialNet::closeService (this-> _registry, "post", this-> _name, this-> _system-> port (), this-> _iface);
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> PostStorageService::onRequest (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", "none");
    if (type == "store") {
      return this-> store (msg);
    } else if (type == "read") {
      return this-> readOne (msg);
    } else {
      return ResponseCode (404);
    }
  }

  std::shared_ptr<config::ConfigNode> PostStorageService::store (const config::ConfigNode & msg) {
    try {
      Post post;
      post.userId = msg ["user_id"].getI ();
      post.userLogin = msg ["user_login"].getStr ();
      post.text = msg ["text"].getStr ();

      match (msg ["tags"]) {
        of (config::Array, a) {
          post.nbTags = a-> getLen () > 16 ? 16 : a-> getLen ();
          for (uint32_t i = 0 ; i < a-> getLen () && i < 16 ; i++) {
            post.tags [i] = (*a) [i].getI ();
          }
        } fo;
      }

      this-> _db.insertPost (post);
      return ResponseCode (200);
    } catch (...) {
      return ResponseCode (400);
    }
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          FINDING         ========================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <collection::ArrayListBase> PostStorageService::onRequestList (const config::ConfigNode & msg) {
    try {
      match (msg ["ids"]) {
        of (config::Array, a) {
          auto res = std::make_shared <collection::CacheArrayList<Post> > ();
          Post * resBuf = new Post[128];
          {
            auto pusher = res-> pusher (0, resBuf, 128);
            Post tmp;
            for (uint32_t i = 0 ; i < a-> getLen () ; i++) {
              if (this-> _db.findPost ((*a)[i].getI (), tmp)) {
                pusher.push (tmp);
              }
            }
          }

          delete[] resBuf;
          return res;
        } fo;
      }
    } catch (...) {}

    return nullptr;
  }

  std::shared_ptr <config::ConfigNode> PostStorageService::readOne (const config::ConfigNode & msg) {
    try {
      Post p;
      if (this-> _db.findPost (msg ["post_id"].getI (), p)) {
        auto result = std::make_shared<config::Dict> ();
        result-> insert ("user_id", std::make_shared <config::Int> (p.userId));
        result-> insert ("user_login", std::make_shared <config::String> (p.userLogin.c_str ()));
        result-> insert ("text", std::make_shared <config::String> (p.text.c_str ()));

        auto tags = std::make_shared <config::Array> ();
        for (uint32_t i = 0 ; i < p.nbTags && i < 16 ; i++) {
          tags-> insert (std::make_shared <config::Int> (p.tags [i]));
        }
        result-> insert ("tags", tags);

        return ResponseCode (200, result);
      } else {
        return ResponseCode (404);
      }
    } catch (...) {
      return ResponseCode (400);
    }
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          STREAMING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void PostStorageService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    auto type = msg.getOr ("type", "none");
    if (type == "posts") {
      this-> streamPosts (stream);
    } else stream.writeU8 (0);
  }

  void PostStorageService::streamPosts (rd_utils::concurrency::actor::ActorStream & stream) {
    Post p;
    while (stream.isOpen ()) {
      if (stream.readOr (0) != 0) {
        auto id = stream.readU32 ();
        if (this-> _db.findPost (id, p)) {
          stream.writeU8 (1);
          stream.writeRaw (p);
        } else {
          stream.writeU8 (0);
          break;
        }
      }
    }
  }

}
