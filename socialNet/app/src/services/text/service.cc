#define __PROJECT__ "TEXT"

#include "service.hh"
#include "../../registry/service.hh"
#include <regex>

using namespace rd_utils::concurrency;
using namespace rd_utils::memory::cache;
using namespace rd_utils::utils;

using namespace socialNet::utils;

namespace socialNet::text {

  TextService::TextService (const std::string & name, actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf) :
    actor::ActorBase (name, sys)
  {
    this-> _registry = socialNet::connectRegistry (sys, conf);
    this-> _iface = conf ["sys"].getOr ("iface", "lo");
  }

  void TextService::onStart () {
    socialNet::registerService (this-> _registry, "text", this-> _name, this-> _system-> port (), this-> _iface);
  }

  void TextService::onMessage (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::POISON_PILL) {
        LOG_INFO ("Registry exit");
        this-> _needClose = false;
        this-> exit ();
      }

      fo;
    }
  }


  void TextService::onQuit () {
    if (this-> _registry != nullptr && this-> _needClose) {
      socialNet::closeService (this-> _registry, "text", this-> _name, this-> _system-> port (), this-> _iface);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          REQUESTS          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr<config::ConfigNode> TextService::onRequest (const config::ConfigNode & msg) {
    match_v (msg.getOr ("type", RequestCode::NONE)) {
      of_v (RequestCode::CTOR_MESSAGE) {
        return this-> constructText (msg);
      }

      elfo {
        return response (ResponseCode::NOT_FOUND);
      }
    }
  }

  std::shared_ptr <config::ConfigNode> TextService::constructText (const config::ConfigNode & msg) {
    try {
      auto text = msg ["text"].getStr ();
      if (text.length () > 560) {
        LOG_INFO ("Throw 14");
        throw std::runtime_error ("Too long");
      }

      auto users = this-> findUserMentions (text);
      auto urlMentions = this-> findUrlMentions (text);

      uint32_t tags [16];
      bool succeed = false;
      auto mentions = this-> transformUserTags (users, tags, succeed);
      if (!succeed) { return response (ResponseCode::SERVER_ERROR); }

      auto shorts = this-> transformUrls (urlMentions, succeed);
      if (!succeed) { return response (ResponseCode::SERVER_ERROR); }


      auto finalText = rd_utils::utils::findAndReplaceAll (text, mentions);
      finalText = rd_utils::utils::findAndReplaceAll (finalText, shorts);

      auto array = std::make_shared <config::Array> ();
      for (uint32_t i = 0 ; i < mentions.size () && i < 16 ; i++) {
        array-> insert (std::make_shared <config::Int> (tags [i]));
      }

      auto result = config::Dict ()
        .insert ("text", finalText)
        .insert ("tags", array);

      return response (ResponseCode::OK, result);
    } catch (std::runtime_error & err) {
      LOG_ERROR ("TextService::constructText ", err.what ());
      return response (ResponseCode::MALFORMED);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ================================          TRANSFORMATIONS          =================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::map <std::string, std::string> TextService::transformUserTags (const std::vector <std::string> & users, uint32_t * tags, bool & succeed) {
    succeed = true;
    std::map <std::string, std::string> resTags;
    try {
      if (users.size () != 0) {
        std::vector <rd_utils::concurrency::actor::ActorRef::RequestFuture> reqs;
        for (uint32_t i = 0 ; i < users.size () && i < 16 ; i++) {
          auto req = config::Dict ()
            .insert ("type", RequestCode::FIND)
            .insert ("login", users [i]);

          auto userService = socialNet::findService (this-> _system, this-> _registry, "user");
          reqs.push_back (userService-> request (req));
        }

        uint32_t i = 0;
        for (auto & req : reqs) {
          auto result = req.wait ();
          if (result && result-> getOr ("code", -1) == ResponseCode::OK) {
            uint32_t id = result-> getOr ("content", 0);
            resTags.emplace ("@" + users [i], "<a href=\"{{FRONT}}/user/" + std::to_string (id) + "\">@" + users [i] + "</a>");
            tags [i] = id;
          }
          i += 1;
        }
      }
    } catch (const std::runtime_error & err) {
      LOG_ERROR ("TextService::transformUserTags ", err.what ());
      succeed = false;
    }

    return resTags;
  }

  std::map <std::string, std::string> TextService::transformUrls (const std::vector <std::string> & urls, bool & succeed) {
    succeed = true;
    std::map <std::string, std::string> result;
    try {
      if (urls.size () != 0) {
        std::vector <std::pair <rd_utils::concurrency::actor::ActorRef::RequestFuture, std::string> > reqs;
        for (auto & url : urls) {
          auto req = config::Dict ()
            .insert ("type", RequestCode::CREATE)
            .insert ("url", std::make_shared <config::String> (url));

          auto urlService = socialNet::findService (this-> _system, this-> _registry, "short_url");
          auto r = urlService-> request (req);
          reqs.push_back ({r, url});
        }

        for (auto & req : reqs) {
          auto u = req.first.wait ();
          if (u && u-> getOr ("code", -1) == 0) {
            auto str = u-> getOr ("content", "");
            result.emplace (req.second, "<a href=\"{{FRONT}}/api/url/" + str + "\">" + str + "</a>");
          }
        }
      }

    } catch (const std::runtime_error & err) {
      LOG_ERROR ("TextService::transformUrls ", err.what ());
      succeed = false;
    }

    return result;
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          FINDING          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::vector <std::string> TextService::findUserMentions (const std::string & msg) {
    std::vector <std::string> names;
    std::regex pattern ("(@)([a-zA-Z0-9-_]+)");
    auto begin = std::sregex_iterator (msg.begin (), msg.end (), pattern);
    while (begin != std::sregex_iterator ()) {
      auto match = (*begin)[2];
      names.push_back (match);
      if (names.size () == 16) break;
      begin ++;
    }

    return names;
  }

  std::vector <std::string> TextService::findUrlMentions (const std::string & msg) {
    std::vector <std::string> names;
    std::regex pattern ("(http://|https://)([a-zA-Z0-9_!~*'().&=+$%-/]+)");
    auto begin = std::sregex_iterator (msg.begin (), msg.end (), pattern);
    while (begin != std::sregex_iterator ()) {
      auto match = (*begin)[2];
      names.push_back (match);
      begin ++;
    }

    return names;
  }

}
