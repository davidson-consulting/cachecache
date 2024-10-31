#pragma once

#include <httpserver.hpp>

namespace socialNet {

  class FrontServer;
  class NewUserRoute : public httpserver::http_resource {
  private:

    FrontServer * _context;

  public:

    NewUserRoute (FrontServer * context);

    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

}
