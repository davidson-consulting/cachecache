#define LOG_LEVEL 10
#define __PROJECT__ "HOME"

#include "login.hh"
#include "service.hh"
#include <nlohmann/json.hpp>
#include "../registry/service.hh"
#include "../services/post/database.hh"
#include "../utils/codes/requests.hh"
#include <rd_utils/_.hh>

using namespace httpserver;
using namespace nlohmann;
using namespace rd_utils::utils;
using namespace socialNet::utils;

namespace socialNet {

  HomeTimelineLenRoute::HomeTimelineLenRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> HomeTimelineLenRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("userId")).c_str ());

      LOG_INFO ("Try get home timeline len : ", userId);

      auto timelineService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "timeline");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::HOME_TIMELINE_LEN)
        .insert ("userId", userId);

      auto result = timelineService-> request (msg).wait ();

      if (result != nullptr && result-> getOr ("code", -1) == ResponseCode::OK) {
        std::string res = std::to_string ((*result)["content"].getI ());
        return std::make_shared <httpserver::string_response> (res, 200, "text/plain");
      }
    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("", 404, "text/plain");
  }


  HomeTimelineRoute::HomeTimelineRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> HomeTimelineRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("user_id")).c_str ());
      int64_t page = std::atoi (std::string (req.get_arg ("page")).c_str ());
      int64_t nb = std::atoi (std::string (req.get_arg ("nb")).c_str ());

      LOG_INFO ("Try get user posts : ", userId, " ", page, " ", nb);

      auto composeService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::HOME_TIMELINE)
        .insert ("userId", userId)
        .insert ("page", page)
        .insert ("nb", nb);

      auto resultStream = composeService-> requestStream (msg).wait ();
      socialNet::post::Post p;
      if (resultStream != nullptr && resultStream-> readU32 () == 200) {
        json result;

        while (resultStream-> isOpen ()) {
          if (resultStream-> readOr (0) == 0) break;
          resultStream-> readRaw (p);

          json post;
          post ["user_id"] = p.userId;
          post ["user_login"] = std::string (p.userLogin);
          post ["text"] = std::string (p.text);
          for (uint32_t i = 0 ; i < p.nbTags ; i++) {
            post["tags"].push_back (p.tags [i]);
          }

          result.push_back (post);
        }

        auto finalResult = result.dump ();
        if (finalResult == "null") {
          return std::make_shared <httpserver::string_response> ("{}", 200, "text/json");
        } else {
          return std::make_shared <httpserver::string_response> (finalResult, 200, "text/json");
        }
      }
    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("", 404, "text/plain");
  }


}
