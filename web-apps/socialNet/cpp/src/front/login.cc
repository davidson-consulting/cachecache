#include "login.hh"
#include "service.hh"

using namespace httpserver;

namespace socialNet {

  LoginRoute::LoginRoute (FrontServer * context) :
    _context (context)
  {}

  std::shared_ptr <http_response> LoginRoute::render (const http_request & req) {
    return std::make_shared <string_response> ("Login", 200, "text/plain");
  }

}
