#include <csignal>

#include <rd_utils/concurrency/_.hh>
#include <string.h>
#include <service/supervisor/supervisor.hh>
#include <service/cachecache.hh>

#include "folly/init/Init.h"
#include "folly/logging/Init.h"
#include "folly/logging/xlog.h"
#include "folly/logging/LogLevel.h"
#include <iostream>

/*void ctrlCHandler(int signum) {
	LOG_DEBUG("Signal ", strsignal(signum), " received");
	generator::exitSignal.emit();
	exit(0);
}*/

using namespace cachecache;

FOLLY_INIT_LOGGING_CONFIG("INFO");

int main (int argc, char ** argv) {
    //folly::init(&argc, &argv, true);

    /*signal(SIGINT, &ctrlCHandler);
    signal(SIGTERM, &ctrlCHandler);
    signal(SIGKILL, &ctrlCHandler);*/
    
 try {
    cachecache::Supervisor supervisor;
    supervisor.configure(argc, argv);
    supervisor.run();
 } catch (std::exception& ex) {
    std::cerr << ex.what() << std::endl;
 } catch (...) {
      std::cerr << "Caught unknown exception." << std::endl;
   }
}
