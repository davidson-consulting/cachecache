#define LOG_LEVEL 10

#include <iostream>
#include <rd_utils/concurrency/actor/_.hh>
#include <cachecache/client/service.hh>

using namespace cachecache;
using namespace cachecache::client;

int main (int argc, char ** argv) {
  CacheService::deploy (argc, argv);
  return 0;
}
