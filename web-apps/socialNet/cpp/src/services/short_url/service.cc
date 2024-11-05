#define __PROJECT__ "SHORT_URL"

#include "service.hh"
#include <cstdlib>
#include <unordered_set>
#include <string>
#include "../../registry/service.hh"

using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet::short_url {

  static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789";


  ShortUrlService::ShortUrlService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf) :
    actor::ActorBase (name, sys)
  {
    srand (time (NULL));
    this-> _db.configure (conf);

    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
    socialNet::registerService (this-> _registry, "short_url", name, sys-> port (), this-> _iface);
  }

  void ShortUrlService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _registry = nullptr;
        this-> exit ();
      }

      fo;
    }
  }

  void ShortUrlService::onQuit () {
    if (this-> _registry != nullptr) {
      socialNet::closeService (this-> _registry, "short_url", this-> _name, this-> _system-> port (), this-> _iface);
    }
  }


  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          FINDING         ========================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> ShortUrlService::onRequest (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::CREATE) {
        return this-> readOrGenerate (msg);
      }

      elof_v (RequestCode::FIND) {
        return this-> find (msg);
      }

      elfo {
        return response (ResponseCode::NOT_FOUND);
      }
    }
  }


  std::shared_ptr <rd_utils::utils::config::ConfigNode> ShortUrlService::find (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      std::string url = msg ["url"].getStr ();

      ShortUrl shUrl;
      ::memset (&shUrl, 0, sizeof (ShortUrl));

      auto shLen = std::min (url.length (), (uint64_t) SHORT_LEN);
      ::memcpy (shUrl.sh, url.c_str (), shLen);

      if (this-> _db.findUrlFromShort (shUrl)) {
        auto res = std::make_shared <config::String> (shUrl.ln);
        return response (ResponseCode::OK, res);
      } else {
        return response (ResponseCode::NOT_FOUND);
      }
    } catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
    }

    return response (ResponseCode::MALFORMED);
  }


  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERTING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <rd_utils::utils::config::ConfigNode> ShortUrlService::readOrGenerate (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      std::string url = msg ["url"].getStr ();

      ShortUrl shUrl;
      ::memset (&shUrl, 0, sizeof (ShortUrl));

      auto lnLen = std::min (url.length (), (uint64_t) LONG_LEN);
      ::memcpy (shUrl.ln, url.c_str (), lnLen);

      if (!this-> _db.findUrl (shUrl)) {
        this-> createShort (shUrl.sh);
        this-> _db.insertUrl (shUrl);
      }

      auto res = std::make_shared <config::String> (shUrl.sh);
      return response (ResponseCode::OK, res);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
    }

    return response (ResponseCode::MALFORMED);
  }

  void ShortUrlService::createShort (char * result) {
    for (uint32_t i = 0 ; i < SHORT_LEN - 1 ; i++) {
      result [i] = base64_chars [::rand () % base64_chars.length ()];
    }

    result [SHORT_LEN - 1] = 0;
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          STREAMING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ShortUrlService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::CREATE) {
        stream.writeU32 (ResponseCode::OK);
        this-> streamCreate (stream);
      }

      elfo {
        stream.writeU32 (ResponseCode::NOT_FOUND);
      }
    }
  }

  void ShortUrlService::streamCreate (actor::ActorStream & stream) {
    while (stream.readOr (0) == 1) {
      auto url = stream.readStr ();
      ShortUrl shUrl;
      ::memset (&shUrl, 0, sizeof (ShortUrl));

      auto lnLen = std::min (url.length (), (uint64_t) LONG_LEN);
      ::memcpy (shUrl.ln, url.c_str (), lnLen);

      if (!this-> _db.findUrl (shUrl)) {
        this-> createShort (shUrl.sh);
        this-> _db.insertUrl (shUrl);
      }

      stream.writeStr (shUrl.sh);
    }
  }


}
