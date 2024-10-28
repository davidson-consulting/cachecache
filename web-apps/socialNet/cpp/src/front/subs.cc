#define LOG_LEVEL 10
#define __PROJECT__ "HOME"

#include "subs.hh"
#include "service.hh"
#include <nlohmann/json.hpp>
#include "../registry/service.hh"
#include "../services/post/database.hh"
#include <rd_utils/_.hh>

using namespace httpserver;
using namespace nlohmann;
using namespace rd_utils::utils;

namespace socialNet {

  SubscriptionLenRoute::SubscriptionLenRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> SubscriptionLenRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("userId")).c_str ());
      std::string jwt = std::string (req.get_arg ("jwt_token"));

      LOG_INFO ("Try get subs len : ", userId);

      auto socialGraphService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "social_graph");
      auto msg = config::Dict ()
        .insert ("type", "count-sub")
        .insert ("userId", userId)
        .insert ("jwt_token", jwt);

      auto result = socialGraphService-> request (msg).wait ();
      std::cout << result-> getOr ("code", -1) << std::endl;
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

  SubscriptionRoute::SubscriptionRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> SubscriptionRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("userId")).c_str ());
      int64_t page = std::atoi (std::string (req.get_arg ("page")).c_str ());
      int64_t nb = std::atoi (std::string (req.get_arg ("nb")).c_str ());
      std::string jwt = std::string (req.get_arg ("jwt_token"));

      LOG_INFO ("Try get user subs : ", userId, " ", page, " ", nb);

      auto composeService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", "subs")
        .insert ("userId", userId)
        .insert ("page", page)
        .insert ("nb", nb)
        .insert ("jwt_token", jwt);

      auto resultStream = composeService-> requestStream (msg).wait ();
      if (resultStream != nullptr) {
        json result;

        while (resultStream-> isOpen ()) {
          if (resultStream-> readOr (0) == 0) break;
          auto rid = resultStream-> readU32 ();
          result.push_back (rid);
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
