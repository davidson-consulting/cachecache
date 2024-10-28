#define LOG_LEVEL 10
#define __PROJECT__ "SERVICES"


#include <iostream>
#include <rd_utils/concurrency/actor/_.hh>
#include <services/_.hh>
#include <registry/service.hh>
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

    auto content = rd_utils::utils::toml::parseFile (filename);
    match (*content) {
      of (rd_utils::utils::config::Dict, cfg) {

        auto port = (*cfg)["sys"]["port"].getI ();

        __GLOBAL_SYSTEM__ = std::make_shared <actor::ActorSystem> (rd_utils::net::SockAddrV4 ("0.0.0.0:" + std::to_string (port)), 8);
        __GLOBAL_SYSTEM__-> start ();
        LOG_INFO ("On port : ", __GLOBAL_SYSTEM__-> port ());

        match ((*cfg)["services"]) {
          of (rd_utils::utils::config::Dict, services) {
            uint32_t i = 0;
            for (auto & ser : services-> getKeys ()) {
              auto nb = (*services) [ser]["nb"].getI ();
              for (uint32_t j = 0 ; j < nb ; j++) {
                if (ser == "user") {
                  __GLOBAL_SYSTEM__->  add <user::UserService> (ser + "_" + std::to_string (i + j), *cfg);
                } else if (ser == "timeline") {
                  __GLOBAL_SYSTEM__->  add <timeline::TimelineService> (ser + "_" + std::to_string (i + j), *cfg);
                } else if (ser == "short") {
                  __GLOBAL_SYSTEM__->  add <short_url::ShortUrlService> (ser + "_" + std::to_string (i + j), *cfg);
                } else if (ser == "text") {
                  __GLOBAL_SYSTEM__->  add <text::TextService> (ser + "_" + std::to_string (i + j), *cfg);
                } else if (ser == "social") {
                  __GLOBAL_SYSTEM__->  add <social_graph::SocialGraphService> (ser + "_" + std::to_string (i + j), *cfg);
                } else if (ser == "compose") {
                  __GLOBAL_SYSTEM__->  add <compose::ComposeService> (ser + "_" + std::to_string (i + j), *cfg);
                } else {
                  throw std::runtime_error ("Unkown service : " + ser);
                }
              }

              i += nb;
            }
          } elfo {
            LOG_ERROR ("Malformed services ", (*cfg)["services"]);
            throw std::runtime_error ("malformed services");
          }
        }

        __GLOBAL_SYSTEM__-> join ();
        __GLOBAL_SYSTEM__-> dispose ();
      } elfo {
        throw std::runtime_error ("Failed to parse config file");
      }
    }
  } catch (std::runtime_error & err) {
    LOG_ERROR ("Error : ", err.what ());
    if (__GLOBAL_SYSTEM__ != nullptr) {
      __GLOBAL_SYSTEM__-> dispose ();
    }
  }

  return 0;
}
