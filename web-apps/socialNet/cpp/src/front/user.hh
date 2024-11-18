#pragma once

#include <httpserver.hpp>
#include <utils/cache/counter.hh>

namespace socialNet {

  class FrontServer;
  class UserTimelineRoute : public httpserver::http_resource {

    FrontServer * _context;
    utils::Counter _cacheCounter;
    utils::Counter _serviceCounter;

  public:

    UserTimelineRoute (FrontServer*);
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

  class UserTimelineLenRoute : public httpserver::http_resource {

    FrontServer * _context;

  public:

    UserTimelineLenRoute (FrontServer * context);

    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

}
