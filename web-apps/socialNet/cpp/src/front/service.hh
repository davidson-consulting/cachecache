#pragma once

#include <httpserver.hpp>
#include <rd_utils/_.hh>
#include "login.hh"

namespace socialNet {

  class FrontServer : public httpserver::http_resource {
  private:

    // The web server used to answer user requests
    std::shared_ptr <httpserver::webserver> _server;

    // The login resource
    LoginRoute _login;

  public:

    FrontServer ();

    /**
     * Configure the server
     */
    void configure (const rd_utils::utils::config::ConfigNode & cfg);

    /**
     * Start the web server
     */
    void start ();

    /**
     * Dispose the web server resources
     */
    void dispose ();

    /**
     * this-> dispose ();
     */
    ~FrontServer ();

  };

}
