#pragma once

#include <httpserver.hpp>
#include <utils/cache/counter.hh>

namespace socialNet {

  class FrontServer;
  class HomeTimelineRoute : public httpserver::http_resource {

    FrontServer * _context;
    utils::Counter _cacheCounter;
    utils::Counter _serviceCounter;

  public:
    HomeTimelineRoute (FrontServer*);

    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

  class HomeTimelineLenRoute : public httpserver::http_resource {

    FrontServer * _context;

  public:

    HomeTimelineLenRoute (FrontServer * context);

    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);

  };

}
