#pragma once

#include <httpserver.hpp>

namespace socialNet {

  class SubmitNewPostRoute : public httpserver::http_resource {
  public:
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

}
