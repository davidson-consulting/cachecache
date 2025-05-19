#pragma once

#include <httpserver.hpp>

namespace socialNet {

  class FrontServer;
  class SubscriptionRoute : public httpserver::http_resource {

    FrontServer* _context;

  public:

    SubscriptionRoute (FrontServer*);
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

  class SubscriptionLenRoute : public httpserver::http_resource {

    FrontServer * _context;

  public:

    SubscriptionLenRoute (FrontServer*);
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);

  };

}
