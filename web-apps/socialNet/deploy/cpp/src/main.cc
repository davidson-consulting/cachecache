#include <iostream>
#include <string>
#include <rd_utils/foreign/CLI11.hh>

#include "deployer/cluster.hh"
#include "deployer/installer.hh"
#include "deployer/app.hh"

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;
using namespace deployer;

auto main(int argc, char *argv[]) -> int {
    CLI::App app;
    std::string hostFile = "./hosts.toml";
    bool install = false;
    app.add_option("-c,--config-path", hostFile, "the path of the configuration file");
    app.add_flag ("-i,--install", install, "install applications on hosts");

    try {
        app.parse (argc, argv);
    } catch (const CLI::ParseError &e) {
        ::exit (app.exit (e));
    }


    deployer::Cluster c;

    auto hostCfg = toml::parseFile (hostFile);
    c.configure (*hostCfg);

    std::map <std::string, std::shared_ptr <deployer::Application> > apps;
    match ((*hostCfg)["deploy"]) {
        of (config::Dict, dc) {
            for (auto & d : dc-> getKeys ()) {
                auto a = std::make_shared <deployer::Application> (c, d);
                a-> configure ((*dc)[d]);
                apps.emplace (d, a);
            }
        } elfo {
            LOG_ERROR ("Malformed configuration file");
        };
    }

    if (install) {
        deployer::Installer i (c);
        i.execute ();
    }

    for (auto & it : apps) {
        LOG_INFO ("Starting app : ", it.first);
        it.second-> start ();
    }

    for (auto & it : apps ) {
        LOG_INFO ("JOINING app : ", it.first);
        it.second-> join ();
    }

    return 0;
}
