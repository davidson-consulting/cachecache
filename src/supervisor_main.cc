#define LOG_LEVEL 10

#include <iostream>
#include <rd_utils/concurrency/actor/_.hh>
#include <cachecache/supervisor/service.hh>

using namespace cachecache;
using namespace cachecache::supervisor;

int main (int argc, char ** argv) {
  SupervisorService::deploy (argc, argv);
  return 0;
}
