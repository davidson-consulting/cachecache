#define LOG_LEVEL 10
#define __PROJECT__ "HOME"

#include "subs.hh"
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

  SubscriptionLenRoute::SubscriptionLenRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> SubscriptionLenRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("user_id")).c_str ());

      LOG_INFO ("Try get subs len : ", userId);

      auto socialGraphService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "social_graph");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::SUBS_LEN)
        .insert ("userId", userId);

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
      int64_t userId = std::atoi (std::string (req.get_arg ("user_id")).c_str ());
      int64_t page = std::atoi (std::string (req.get_arg ("page")).c_str ());
      int64_t nb = std::atoi (std::string (req.get_arg ("nb")).c_str ());

      LOG_INFO ("Try get user subs : ", userId, " ", page, " ", nb);

      auto composeService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::SUBSCIRPTIONS)
        .insert ("userId", userId)
        .insert ("page", page)
        .insert ("nb", nb);

      auto resultStream = composeService-> requestStream (msg).wait ();
      if (resultStream != nullptr && resultStream-> readU32 () == 200) {
        json result;

        while (resultStream-> isOpen ()) {
          if (resultStream-> readOr (0) == 0) break;
          auto rid = resultStream-> readU32 ();
          auto name = resultStream-> readStr ();

          json sub;
          sub ["user_id"] = rid;
          sub ["login"] = name;
          result.push_back (sub);
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
