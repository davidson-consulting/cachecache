#define LOG_LEVEL 10
#define __PROJECT__ "MAIN"

#include <csignal>
#include <iostream>
#include <front/service.hh>
#include <rd_utils/_.hh>
#include <rd_utils/foreign/CLI11.hh>

socialNet::FrontServer server;

void ctrlCHandler (int signum) {
  LOG_INFO ("Signal ", strsignal(signum), " received");
  server.dispose ();

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

    server.configure (*cfg);
    server.start ();
  } catch (std::runtime_error & err) {
    LOG_ERROR ("Error : ", err.what ());
  }
}
