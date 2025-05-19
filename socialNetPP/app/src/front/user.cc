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
    // , _cacheCounter ("user_cache")
    // , _serviceCounter ("user_service")
  {}

  std::shared_ptr <http_response> UserTimelineRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("user_id")).c_str ());
      int64_t page = std::atoi (std::string (req.get_arg ("page")).c_str ());
      int64_t nb = std::atoi (std::string (req.get_arg ("nb")).c_str ());

      std::string rqt;
      rd_utils::concurrency::timer t;
      if (this-> _context-> getCache () != nullptr) {
        rqt = std::string ("user/") + std::string (req.get_arg ("user_id")) + "/" + std::string (req.get_arg ("page")) + "/" + std::string (req.get_arg ("nb"));
        std::string result;
        if (this-> _context-> getCache ()-> get (rqt, result)) {
          // this-> _cacheCounter.insert (t.time_since_start ());
          return std::make_shared <httpserver::string_response> (result, 200, "text/json");
        }

        // this-> _cacheCounter.insert (t.time_since_start ());
      }

      t.reset ();
      auto composeService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::USER_TIMELINE)
        .insert ("userId", userId)
        .insert ("page", page)
        .insert ("nb", nb);

      auto resp = composeService-> request (msg, 100).wait ();
      if (resp && resp-> getOr ("code", -1) == 200) {
        match ((*resp) ["content"]) {
          of (config::Array, arr) {
            json result;
            for (uint32_t i = 0 ; i < arr-> getLen () ; i++) {
              json post;
              post ["user_id"] = (*arr) [i]["userId"].getI ();
              post ["post_id"] = (*arr) [i]["postId"].getI ();
              post ["user_login"] = (*arr) [i]["userLogin"].getStr ();
              post ["text"] = (*arr) [i]["text"].getStr ();
              match ((*arr)[i]["tags"]) {
                of (config::Array, tags) {
                  for (uint32_t t = 0 ; t < tags-> getLen () ; t++) {
                    post["tags"].push_back ((*tags)[t].getI ());
                  }
                } fo;
              }

              result.push_back (post);
            }

            auto finalResult = result.dump ();
            if (finalResult == "null") {
              finalResult = "{}";
            } else if (this-> _context-> getCache () != nullptr) {
              this-> _context-> getCache ()-> set (rqt, reinterpret_cast <const uint8_t*> (finalResult.c_str ()), finalResult.length ());
            }

            // this-> _serviceCounter.insert (t.time_since_start ());
            return std::make_shared <httpserver::string_response> (finalResult, 200, "text/json");
          }
        } fo;
      }

      if (resp != nullptr) {
        return std::make_shared <httpserver::string_response> ("", resp-> getOr ("code", 500), "text/plain");
      }

    } catch (const std::runtime_error & e) {
      LOG_ERROR ("Failed for request : ", e.what (), req);
    }

    return std::make_shared <httpserver::string_response> ("", 404, "text/plain");
  }

}
