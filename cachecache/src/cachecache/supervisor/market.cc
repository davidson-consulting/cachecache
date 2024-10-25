#define LOG_LEVEL 10

#include <rd_utils/utils/log.hh>
#include "market.hh"

using namespace rd_utils::utils;

namespace cachecache::supervisor {

  Market::Market (MemorySize size) :
    _size (size)
  {}

  Market::~Market () {}

  void Market::registerCache (uint64_t uid, MemorySize req, MemorySize usage) {
    if (this-> _entities.find (uid) != this-> _entities.end ()) {
      this-> _entities.erase (uid);
    }

    this-> _entities.emplace (uid, CacheStatus {
        .uid = uid,
        .req = req,
        .size = usage,
        .usage = usage,
        .last = usage,
        .wallet = 0
      });
  }

  void Market::removeCache (uint64_t uid) {
    this-> _entities.erase (uid);
  }

  void Market::updateUsage (uint64_t uid, MemorySize usage) {
    auto it = this-> _entities.find (uid);
    if (it != this-> _entities.end ()) {
      it-> second.usage = usage;
      it-> second.last = it-> second.size;
    }
  }

  MemorySize Market::getCacheSize (uint64_t uid) const {
    auto it = this-> _entities.find (uid);
    if (it != this-> _entities.end ()) {
      return it-> second.size;
    }

    return MemorySize::B (0);
  }

  bool Market::hasChanged (uint64_t uid) const {
    auto it = this-> _entities.find (uid);
    if (it != this-> _entities.end ()) {
      return (it-> second.size.bytes () != it-> second.last.bytes ());
    }

    return false;
  }


  void Market::run () {
    LOG_INFO ("DOING NOTHING.....");
    for (auto & it : this-> _entities) {
      it.second.size = it.second.req;
    }
  }

}
