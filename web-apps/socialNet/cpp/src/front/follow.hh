#pragma once

#include <httpserver.hpp>

namespace socialNet {

  class FollowerRoute : public httpserver::http_resource {
  public:
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

  class FollowerLenRoute : public httpserver::http_resource {
  public:
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

  class FollowRoute : public httpserver::http_resource {
  public:
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

  class UnfollowRoute : public httpserver::http_resource {
  public:
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

  class IsFollowingRoute : public httpserver::http_resource {
  public:
    std::shared_ptr <httpserver::http_response> render (const httpserver::http_request & req);
  };

}
