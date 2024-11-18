#define LOG_LEVEL 4

#include <random>
#include <rd_utils/utils/_.hh>

#include <application.hh>

using namespace cachecache_sim;
using namespace std;

CacheApplication::CacheApplication(string addr, int port, int cost_get, int cost_set, GeneratorConfiguration& genCfg): 
    _cacheClient(addr, port)
    , _generator(genCfg) {
  LOG_INFO("Init application cache - Configured for cache on address ", addr, ":", port);

  this->_cost_per_operation[CACHE_OPERATION::GET] = cost_get;
  this->_cost_per_operation[CACHE_OPERATION::SET] = cost_set;

  this->_count_operations[CACHE_OPERATION::GET] = 0;
  this->_count_operations[CACHE_OPERATION::SET] = 0;
}

CacheApplication::CacheApplication(std::string addr, int port, int cost_get, int cost_set):
  _cacheClient(addr, port) {
  LOG_INFO("Init application cache - Configured for cache on address ", addr, ":", port);

  this->_cost_per_operation[CACHE_OPERATION::GET] = cost_get;
  this->_cost_per_operation[CACHE_OPERATION::SET] = cost_set;

  this->_count_operations[CACHE_OPERATION::GET] = 0;
  this->_count_operations[CACHE_OPERATION::SET] = 0;
}

CacheApplication::~CacheApplication() {
  int gets = this->_count_operations[CACHE_OPERATION::GET];
  int sets = this->_count_operations[CACHE_OPERATION::SET];
  LOG_INFO("Done. Total extra flops = ", this->_extra_flops, " - Gets ", gets, " Sets ", sets, " Hits ", this->_hits, " Misses ", this->_misses);
}

/*
 * Execute the cache application. It goes as follows:
 * 1) check for unexecuted_flops from previous iteration 
 */
uint32_t CacheApplication::execute(uint32_t tid, uint32_t nbFlops_needed, uint32_t nbFlops_max) { 
  //LOG_INFO("NB flops needed ", nbFlops_needed, " Max ", nbFlops_max);
  // Register thread if not already done
  if (this->_time_executed.find(tid) == this->_time_executed.end()) {
    this->_time_executed[tid] = 0;
    this->_time_simulated[tid] = 0;
  }

  // Look for unexecuted flops. If there is any, deduce it from tid's simulated
  // time
  //LOG_INFO("Unexecuted flops for tid ", tid, " = ", this->unexecuted_flops(tid));
  uint32_t unexecuted_flops = this->unexecuted_flops(tid);
  if (unexecuted_flops > 0) {
    this->_time_simulated[tid] -= unexecuted_flops;
    this->acknowledged_unexecuted(tid);
  }

  // if simulated time is lower than executed time, deduce the difference from 
  // flops to be executed in order to synchronize both counters
  uint32_t actualFlops_needed = nbFlops_needed;
  if (this->_time_simulated[tid] < this->_time_executed[tid]) {
    // the diff between simulated and executed time could be higher 
    // than flops needed
    uint32_t diff = min(this->_time_executed[tid] - this->_time_simulated[tid], actualFlops_needed); 
    this->_time_simulated[tid] += diff;
    actualFlops_needed -= diff;
  }
  //LOG_INFO("Flops actually needed ", actualFlops_needed);

  Trace trace = this->_generator.next_operation();
 
  int extra = 0;
  int executed = 0;
  int ops = 0;
  while (executed < actualFlops_needed && executed + this->_cost_per_operation[trace.operation] < nbFlops_max) {
    executed += this->_cost_per_operation[trace.operation];
    this->_count_operations[trace.operation]++;

    if (trace.operation == CACHE_OPERATION::GET) {
      string value;

      if (!this->_cacheClient.get(trace.key, value)) {
        ops++;
        executed += this->_cost_per_operation[CACHE_OPERATION::SET]; 
        extra += this->_cost_per_operation[CACHE_OPERATION::SET]; 
        value = this->_generator.gen_key(trace.value_size);
        this->_cacheClient.set(trace.key, value);

        this->_misses++;
      } else {
        this->_hits++;
      }

    } else {
      string value = this->_generator.gen_key(trace.value_size);
      this->_cacheClient.set(trace.key, value);
    }

    ops++;
    trace = this->_generator.next_operation();
  } 

  //LOG_INFO("Executed ", ops, " operations");
  //LOG_INFO("Time simulated ", this->_time_simulated[tid], " - executed ", this->_time_executed[tid], " - Extra flops ", extra);
  this->_time_executed[tid] += executed + extra;
  this->_time_simulated[tid] += executed;
  this->_extra_flops += extra;
  return extra;
}


