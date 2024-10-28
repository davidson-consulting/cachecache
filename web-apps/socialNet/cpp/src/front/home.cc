#define LOG_LEVEL 10
#define __PROJECT__ "HOME"

#include "login.hh"
#include "service.hh"
#include <nlohmann/json.hpp>
#include "../registry/service.hh"
#include <rd_utils/_.hh>

using namespace httpserver;
using namespace nlohmann;
using namespace rd_utils::utils;

namespace socialNet {

  HomeTimelineLenRoute::HomeTimelineLenRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> HomeTimelineLenRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("userId")).c_str ());
      std::string jwt = std::string (req.get_arg ("jwt_token"));

      std::cout << userId << " " << jwt << std::endl;
      LOG_INFO ("Try get home timeline len : ", userId);

      auto timelineService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "timeline");
      auto msg = config::Dict ()
        .insert ("type", "count-home")
        .insert ("userId", userId)
        .insert ("jwt_token", jwt);

      auto result = timelineService-> request (msg).wait ();
      if (result != nullptr && result-> getOr ("code", -1) == 200) {
        std::string res = std::to_string ((*result)["content"].getI ());
        return std::make_shared <httpserver::string_response> (res, 200, "text/plain");
      }
    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("", 404, "text/plain");
  }

}
