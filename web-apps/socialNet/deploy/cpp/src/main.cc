#include <iostream>
#include <string>
#include <rd_utils/foreign/CLI11.hh>

#include "deployer/cluster.hh"
#include "deployer/installer.hh"

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;
using namespace deployer;

std::string appOption (int argc, char ** argv) {
    CLI::App app;
    std::string filename = "./config.toml";
    app.add_option("-c,--config-path", filename, "the path of the configuration file");
    try {
        app.parse (argc, argv);
    } catch (const CLI::ParseError &e) {
        ::exit (app.exit (e));
    }

    return filename;
}

auto main(int argc, char *argv[]) -> int {
    auto configFile = appOption (argc, argv);
    auto cfg = toml::parseFile (configFile);

    deployer::Cluster c;
    c.configure (*cfg);

    deployer::Installer i (c, utils::parent_directory (utils::get_absolute_path (configFile)));
    i.execute (*cfg);

    return 0;
}
