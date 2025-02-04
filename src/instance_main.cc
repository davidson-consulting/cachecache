#define LOG_LEVEL 10

#include <iostream>
#include <rd_utils/concurrency/actor/_.hh>
#include "instance/service.hh"

using namespace kv_store;
using namespace kv_store::instance;

int main (int argc, char ** argv) {
  CacheService::deploy (argc, argv);
  return 0;
}
