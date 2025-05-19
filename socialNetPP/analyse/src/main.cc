#include "analyser/analyser.hh"

auto main(int argc, char *argv[]) -> int {
    analyser::Analyser a;

    a.configure (argc, argv);
    a.execute ();

    return 0;
}
