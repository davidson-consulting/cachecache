#define LOG_LEVEL 10
#define __PROJECT__ "NEW_USER"

#include "newuser.hh"
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

  NewUserRoute::NewUserRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> NewUserRoute::render (const http_request & req) {
    try {
      auto js = json::parse (req.get_content ());

      auto login = js ["login"].get <std::string> ();
      auto pass = js ["password"].get <std::string> ();
      LOG_INFO ("Try new user : ", login, " ", pass);

      auto userService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", RequestCode::REGISTER_USER)
        .insert ("login", login)
        .insert ("password", pass);

      auto result = userService-> request (msg).wait ();
      if (result != nullptr && result-> getOr ("code", -1) == 200) {
        return std::make_shared <httpserver::string_response> ("OK", 200, "text/json");
      }

      if (result != nullptr) {
        return std::make_shared <httpserver::string_response> ("", result-> getOr ("code", 500), "text/plain");
      }
    } catch (...) {}

    return std::make_shared <httpserver::string_response> ("OK", 400, "text/plain");
  }

}
