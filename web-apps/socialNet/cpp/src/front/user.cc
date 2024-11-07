#define __PROJECT__ "HOME"

#include "user.hh"
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

  UserTimelineLenRoute::UserTimelineLenRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> UserTimelineLenRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("user_id")).c_str ());
      LOG_DEBUG ("Try get user posts len : ", userId);

      auto timelineService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "timeline");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::USER_TIMELINE_LEN)
        .insert ("userId", userId);

      auto result = timelineService-> request (msg).wait ();
      if (result != nullptr && result-> getOr ("code", -1) == 200) {
        std::string res = std::to_string ((*result)["content"].getI ());
        return std::make_shared <httpserver::string_response> (res, 200, "text/plain");
      }

      if (result != nullptr) {
        return std::make_shared <httpserver::string_response> ("", result-> getOr ("code", 500), "text/plain");
      }
    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("", 404, "text/plain");
  }

  UserTimelineRoute::UserTimelineRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> UserTimelineRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("user_id")).c_str ());
      int64_t page = std::atoi (std::string (req.get_arg ("page")).c_str ());
      int64_t nb = std::atoi (std::string (req.get_arg ("nb")).c_str ());

      std::string rqt;
      if (this-> _context-> getCache () != nullptr) {
        rqt = std::string ("user/") + std::string (req.get_arg ("user_id")) + "/" + std::string (req.get_arg ("page")) + "/" + std::string (req.get_arg ("nb"));
        std::string result;
        if (this-> _context-> getCache ()-> get (rqt, result)) {
          return std::make_shared <httpserver::string_response> (result, 200, "text/json");
        }
      }


      auto composeService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::USER_TIMELINE)
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
          post ["userId"] = p.userId;
          post ["userLogin"] = std::string (p.userLogin);
          post ["text"] = std::string (p.text);
          for (uint32_t i = 0 ; i < p.nbTags ; i++) {
            post["tags"].push_back (p.tags [i]);
          }

          result.push_back (post);
        }

        auto finalResult = result.dump ();
        if (finalResult == "null") {
          finalResult = "{}";
        } else if (this-> _context-> getCache () != nullptr) {
          this-> _context-> getCache ()-> set (rqt, reinterpret_cast <const uint8_t*> (finalResult.c_str ()), finalResult.length ());
        }

        return std::make_shared <httpserver::string_response> (finalResult, 200, "text/json");
      }
    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("", 404, "text/plain");
  }

}
