#define LOG_LEVEL 10
#define __PROJECT__ "HOME"

#include "user.hh"
#include "service.hh"
#include <nlohmann/json.hpp>
#include "../registry/service.hh"
#include <rd_utils/_.hh>

using namespace httpserver;
using namespace nlohmann;
using namespace rd_utils::utils;

namespace socialNet {

  UserTimelineLenRoute::UserTimelineLenRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> UserTimelineLenRoute::render (const http_request & req) {
    try {
      int64_t userId = std::atoi (std::string (req.get_arg ("userId")).c_str ());
      LOG_INFO ("Try get user posts len : ", userId);

      auto timelineService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "timeline");
      auto msg = config::Dict ()
        .insert ("type", "count-posts")
        .insert ("userId", userId);

      auto result = timelineService-> request (msg).wait ();
      if (result != nullptr && result-> getOr ("code", -1) == 200) {
        std::string res = std::to_string ((*result)["content"].getI ());
        return std::make_shared <httpserver::string_response> (res, 200, "text/plain");
      }
    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("", 404, "text/plain");
  }

}
