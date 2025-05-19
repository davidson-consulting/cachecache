#pragma once

#include <httpserver.hpp>

namespace socialNet {

  class FrontServer;
  class LoginRoute : public httpserver::http_resource {
  private:

    FrontServer * _context;

  public:

    LoginRoute (FrontServer * context);

    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

}
