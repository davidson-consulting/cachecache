#include "deployer/deployment.hh"

auto main(int argc, char *argv[]) -> int {
    deployer::Deployment d;

    d.configure (argc, argv);
    d.start ();
    d.join ();

    return 0;
}
