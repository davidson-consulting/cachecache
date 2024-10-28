#include "newuser.hh"
#include "service.hh"
#include <nlohmann/json.hpp>
#include "../registry/service.hh"
#include <rd_utils/_.hh>

using namespace httpserver;
using namespace nlohmann;
using namespace rd_utils::utils;

namespace socialNet {

  NewUserRoute::NewUserRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> NewUserRoute::render (const http_request & req) {
    std::cout << req.get_content () << std::endl;
    auto js = json::parse (req.get_content ());
    std::cout << js << std::endl;

    auto login = js ["login"].get <std::string> ();
    auto pass = js ["password"].get <std::string> ();
    LOG_INFO ("Try new user : ", login, " ", pass);

    auto userService = socialNet::findService (this-> _context-> getSystem (), this-> _context-> getRegistry (), "compose");
    auto msg = config::Dict ()
      .insert ("type", "register")
      .insert ("login", login)
      .insert ("password", pass);

    auto result = userService-> request (msg).wait ();
    if (result != nullptr && result-> getOr ("code", -1) == 200) {
      return std::make_shared <httpserver::string_response> ("OK", 200, "text/plain");
    }

    return std::make_shared <httpserver::string_response> ("OK", 400, "text/plain");
  }

}
