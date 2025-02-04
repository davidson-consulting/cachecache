#define LOG_LEVEL 10

#include <iostream>
#include <rd_utils/concurrency/actor/_.hh>
#include "supervisor/service.hh"

using namespace kv_store;
using namespace kv_store::supervisor;

int main (int argc, char ** argv) {
  SupervisorService::deploy (argc, argv);
  return 0;
}
