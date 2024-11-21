#pragma once

#include <httpserver.hpp>

namespace socialNet {

  class FrontServer;
  class SubmitNewPostRoute : public httpserver::http_resource {
  private:

    FrontServer * _context;

  public:

    SubmitNewPostRoute (FrontServer * context);

    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

}
