#define LOG_LEVEL 10

#include <iostream>
#include <rd_utils/concurrency/actor/_.hh>
#include <cachecache/instance/service.hh>

using namespace cachecache;
using namespace cachecache::instance;

int main (int argc, char ** argv) {
  CacheService::deploy (argc, argv);
  return 0;
}
