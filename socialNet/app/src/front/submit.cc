#define __PROJECT__ "SUBMIT"

#include "submit.hh"
#include "service.hh"
#include <nlohmann/json.hpp>
#include "../registry/service.hh"
#include "../utils/codes/requests.hh"
#include <rd_utils/_.hh>

using namespace httpserver;
using namespace nlohmann;
using namespace rd_utils::utils;
using namespace socialNet::utils;

namespace socialNet {

  SubmitNewPostRoute::SubmitNewPostRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> SubmitNewPostRoute::render (const http_request & req) {
    try {
      auto js = json::parse (req.get_content ());
      auto jwt = js ["token"].get <std::string> ();
      auto uid = js ["user_id"].get <int64_t> ();
      auto content = js ["text"].get <std::string> ();

      LOG_DEBUG ("User : ", uid, " submitting post");

      auto composeService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::SUBMIT_POST)
        .insert ("jwt_token", jwt)
        .insert ("userId", (int64_t) uid)
        .insert ("content", content);

      rd_utils::concurrency::timer t;
      auto result = composeService-> request (msg).wait ();
      if (t.time_since_start () > 1) {
        std::cout << msg << std::endl;
      }

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
