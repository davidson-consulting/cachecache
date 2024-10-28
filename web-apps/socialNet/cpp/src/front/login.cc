#define LOG_LEVEL 10
#define __PROJECT__ "LOGIN"

#include "login.hh"
#include "service.hh"
#include <nlohmann/json.hpp>
#include "../registry/service.hh"
#include <rd_utils/_.hh>

using namespace httpserver;
using namespace nlohmann;
using namespace rd_utils::utils;

namespace socialNet {

  LoginRoute::LoginRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> LoginRoute::render (const http_request & req) {
    try {
      auto js = json::parse (req.get_content ());

      auto login = js ["login"].get <std::string> ();
      auto pass = js ["password"].get <std::string> ();
      LOG_INFO ("Try login : ", login, " ", pass);

      auto userService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
      auto msg = config::Dict ()
        .insert ("type", "login")
        .insert ("login", login)
        .insert ("password", pass);

      auto result = userService-> request (msg).wait ();
      if (result != nullptr && result-> getOr ("code", -1) == 200) {
        std::cout << *result << std::endl;
        json j;
        j ["jwt_token"] = (*result)["content"]["jwt_token"].getStr ();
        j ["userId"] = (*result)["content"]["userId"].getI ();

        return std::make_shared <httpserver::string_response> (j.dump (), 200, "text/json");
      }

      if (result != nullptr) {
        return std::make_shared <httpserver::string_response> ("", result-> getOr ("code", 500), "text/plain");
      }
    } catch (...) {}

    return std::make_shared <httpserver::basic_auth_fail_response> ("FAIL", "test@example.com");
  }

}
