#pragma once

#include <httpserver.hpp>
#include <rd_utils/_.hh>
#include "login.hh"
#include "newuser.hh"

namespace socialNet {

  class FrontServer : public httpserver::http_resource {
  private:

    // The web server used to answer user requests
    std::shared_ptr <httpserver::webserver> _server;

    // The login resource
    LoginRoute _login;

    // The logon resource
    NewUserRoute _logon;

    // The actor system used by the front to communicate with registry
    std::shared_ptr <rd_utils::concurrency::actor::ActorSystem> _sys;

    // The connection to the registry
    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;

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
     * @returns: the actor sytem used by the front end
     */
    rd_utils::concurrency::actor::ActorSystem* getSystem ();

    /**
     * @returns: the actor reference to the registry
     */
    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> getRegistry ();

    /**
     * this-> dispose ();
     */
    ~FrontServer ();

  };

}
