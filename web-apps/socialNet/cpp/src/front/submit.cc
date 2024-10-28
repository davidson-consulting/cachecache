#define LOG_LEVEL 10
#define __PROJECT__ "SUBMIT"

#include "submit.hh"
#include "service.hh"
#include <nlohmann/json.hpp>
#include "../registry/service.hh"
#include <rd_utils/_.hh>

using namespace httpserver;
using namespace nlohmann;
using namespace rd_utils::utils;

namespace socialNet {

  SubmitNewPostRoute::SubmitNewPostRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> SubmitNewPostRoute::render (const http_request & req) {
    try {
      auto js = json::parse (req.get_content ());

      auto jwt = js ["jwt_token"].get <std::string> ();
      auto uid = js ["userId"].get <int64_t> ();
      auto content = js ["content"].get <std::string> ();

      LOG_INFO ("User : ", uid, " submitting post : ", content);

      auto userService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", "submit")
        .insert ("jwt_token", jwt)
        .insert ("userId", (int64_t) uid)
        .insert ("content", content);

      auto result = userService-> request (msg).wait ();
      if (result != nullptr && result-> getOr ("code", -1) == 200) {
        return std::make_shared <httpserver::string_response> ("OK", 200, "text/plain");
      }

      if (result != nullptr) {
        return std::make_shared <httpserver::string_response> ("", result-> getOr ("code", 500), "text/plain");
      }
    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("OK", 400, "text/plain");
  }

}
