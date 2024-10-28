#define LOG_LEVEL 10
#define __PROJECT__ "REGISTRY"

#include <iostream>
#include <rd_utils/concurrency/actor/_.hh>
#include <registry/service.hh>
#include <rd_utils/_.hh>
#include <csignal>
#include <rd_utils/foreign/CLI11.hh>

using namespace rd_utils;
using namespace rd_utils::concurrency;
using namespace socialNet;


std::shared_ptr <concurrency::actor::ActorSystem> __GLOBAL_SYSTEM__;

void ctrlCHandler (int signum) {
  LOG_INFO ("Signal ", strsignal(signum), " received");
  if (__GLOBAL_SYSTEM__ != nullptr) {
    __GLOBAL_SYSTEM__-> dispose ();
  }

  LOG_INFO ("Exiting");
  ::exit (0);
}

auto main(int argc, char *argv[]) -> int {
  try {
    ::signal (SIGINT, &ctrlCHandler);
    ::signal (SIGTERM, &ctrlCHandler);
    ::signal (SIGKILL, &ctrlCHandler);

    CLI::App app;
    std::string filename = "./config.toml";
    app.add_option("-c,--config-path", filename, "the path of the configuration file");
    try {
      app.parse (argc, argv);
    } catch (const CLI::ParseError &e) {
      ::exit (app.exit (e));
    }

    auto cfg = rd_utils::utils::toml::parseFile (filename);
    auto port = (*cfg)["sys"]["port"].getI ();

    __GLOBAL_SYSTEM__ = std::make_shared <actor::ActorSystem> (rd_utils::net::SockAddrV4 ("0.0.0.0:" + std::to_string (port)), 8);
    __GLOBAL_SYSTEM__-> start ();

    LOG_INFO ("On port : ", __GLOBAL_SYSTEM__-> port ());

    __GLOBAL_SYSTEM__-> add <socialNet::RegistryService> ("registry");

    // Deployer deployer;
    // deployer.deploy (*cfg);

    __GLOBAL_SYSTEM__-> join ();
    __GLOBAL_SYSTEM__-> dispose ();
  } catch (std::runtime_error & err) {
    LOG_ERROR ("Error : ", err.what ());
  }

  return 0;
}
