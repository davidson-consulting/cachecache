#include <iostream>
#include <string>
#include <rd_utils/foreign/CLI11.hh>
#include "deployer/deployment.hh"

using namespace rd_utils;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;
using namespace deployer;

auto main(int argc, char *argv[]) -> int {
    deployer::Deployment d;

    d.configure (argc, argv);
    d.start ();
    d.join ();

    return 0;
}
