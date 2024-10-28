#pragma once

#include <httpserver.hpp>

namespace socialNet {

  class HomeTimelineRoute : public httpserver::http_resource {
  public:
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

  class HomeTimelineLenRoute : public httpserver::http_resource {
  public:
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

}
