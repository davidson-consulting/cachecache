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

      LOG_DEBUG ("Try get subs len : ", userId);

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

      LOG_DEBUG ("Try get user subs : ", userId, " ", page, " ", nb);
      auto composeService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::SUBSCIRPTIONS)
        .insert ("userId", userId)
        .insert ("page", page)
        .insert ("nb", nb);

      auto resp = composeService-> request (msg).wait ();
      if (resp && resp-> getOr ("code", -1) == ResponseCode::OK) {
        match ((*resp)["content"]) {
          of (config::Array, arr) {
            json result;
            for (uint64_t i = 0 ; i < arr-> getLen () ; i++) {
              json sub;
              sub ["user_id"] = (*arr)[i]["id"].getStr ();
              sub ["login"] = (*arr)[i]["login"].getStr ();
              result.push_back (sub);
            }

            auto finalResult = result.dump ();
            if (finalResult == "null") {
              return std::make_shared <httpserver::string_response> ("{}", 200, "text/json");
            } else {
              return std::make_shared <httpserver::string_response> (finalResult, 200, "text/json");
            }
          }
        } fo;
      }

      if (resp != nullptr) {
        return std::make_shared <httpserver::string_response> ("", resp-> getOr ("code", 500), "text/plain");
      }

    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("", 404, "text/plain");
  }


}
