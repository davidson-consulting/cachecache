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


  void ShortUrlService::onQuit () {
    socialNet::closeService (this-> _registry, "short_url", this-> _name, this-> _system-> port (), this-> _iface);
  }


  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          FINDING         ========================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> ShortUrlService::onRequest (const config::ConfigNode & msg) {
    auto type = msg.getOr ("type", "none");
    if (type == "create") {
      return this-> readOrGenerate (msg);
    } else if (type == "find") {
      return ResponseCode (500);
    } else {
      return ResponseCode (404);
    }
  }


  std::shared_ptr <rd_utils::utils::config::ConfigNode> ShortUrlService::find (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      std::string url = msg ["url"].getStr ();

      ShortUrl shUrl = {.sh = url, .ln = ""};
      if (this-> _db.findUrlFromShort (shUrl)) {
        auto res = std::make_shared <config::String> (shUrl.ln.data ());
        return ResponseCode (200, res);
      } else {
        return ResponseCode (404);
      }
    } catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
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

  std::shared_ptr <rd_utils::utils::config::ConfigNode> ShortUrlService::readOrGenerate (const rd_utils::utils::config::ConfigNode & msg) {
    try {
      std::string url = msg ["url"].getStr ();

      ShortUrl shUrl = {.sh = "", .ln = url};
      if (!this-> _db.findUrl (shUrl)) {
        auto sh = this-> createShort ();
        shUrl.sh = sh;
        this-> _db.insertUrl (shUrl);
      }

      auto res = std::make_shared <config::String> (shUrl.sh.data ());
      return ResponseCode (200, res);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("Error : ", err.what ());
    }

    return ResponseCode (400);
  }

  rd_utils::memory::cache::collection::FlatString<16> ShortUrlService::createShort () {
    rd_utils::memory::cache::collection::FlatString<16> result;
    for (uint32_t i = 0 ; i < 15 ; i++) {
      result.data ()[i] = base64_chars [::rand () % base64_chars.length ()];
    }

    result.data ()[15] = '\0';
    result.forceLen (16);
    return result;
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * =================================          STREAMING         =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ShortUrlService::onStream (const config::ConfigNode & msg, actor::ActorStream & stream) {
    auto type = msg.getOr ("type", "none");
    if (type == "create") {
      this-> streamCreate (stream);
    } else stream.writeU8 (0);
  }

  void ShortUrlService::streamCreate (actor::ActorStream & stream) {
    while (stream.isOpen ()) {
      auto n = stream.readOr (0);
      if (n != 0) {
        auto url = stream.readStr ();
        ShortUrl shUrl = {.sh = "", .ln = url};
        if (!this-> _db.findUrl (shUrl)) {
          auto sh = this-> createShort ();
          shUrl.sh = sh;
          std::cout << shUrl.sh << std::endl;
          this-> _db.insertUrl (shUrl);
        }

        stream.writeStr (shUrl.sh.data ());
      } else break;
    }
  }


}
