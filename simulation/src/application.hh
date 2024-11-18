#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

#include <vcpu_sim/virtual/application.hh>
#include <cachecache/client/service.hh>
#include "generator.hh"

using namespace cachecache::client;

namespace cachecache_sim {
   // Application simulating an application using a cache system, communicating 
  // with a running instance of cachecache
  class CacheApplication: public vcpu_sim::virt::Application {
    public:
      CacheApplication(std::string addr, int port, int cost_get, int cost_set);
      CacheApplication(std::string addr, int port, int cost_get, int cost_set, GeneratorConfiguration& generatorCfg);
      ~CacheApplication();

      /**
       * Execute "nbFlops" instructions in the
       * application
       *
       * 
       *
       * @assume: nbFlops_needed <= nbFlops_max
       * @params:
       *      - nbFlops_needed: number of flops to execute
       *      - nbFlops_max: number of flops to not exeed (in case of extra)
       * @returns: A number of extra-instructions to be executed later. Can 
       *      be used in case of the execution of the application leads
       *      to unpredicted load (e.g.: for a cache application, 
       *      a miss on a get will lead to a put).
       */
      uint32_t execute(uint32_t tid, uint32_t nbFlops_needed, uint32_t nbFlops_max) override;

    private:
      // Time per tid executed by the application
      std::unordered_map<uint32_t, uint32_t> _time_executed;
      // How many instructions where executed by the simulation ?
      // Can be lower than _flop_cursor if some instructions where 
      // not executed by the simulation
 
      std::unordered_map<uint32_t, uint32_t> _time_simulated;

      // Client for the cache
      CacheClient _cacheClient;

      Generator _generator;

      // cost in flop for a given instruction
      std::unordered_map<CACHE_OPERATION, int> _cost_per_operation;
      
      //
      // Statistics about the whole execution
      //
      int _extra_flops = 0;
      std::unordered_map<CACHE_OPERATION, int> _count_operations;
      int _hits = 0;
      int _misses = 0;
  };
}
