#pragma once

#include <vector>
#include <random>
#include <map>
#include <cstdint>

#include <rd_utils/utils/config/_.hh>
#include <vcpu_sim/virtual/_.hh>
#include <vcpu_sim/physical/_.hh>
#include <vcpu_sim/config/_.hh>
#include <vcpu_sim/export/_.hh>

#include "generator.hh"

using namespace vcpu_sim; 

namespace cachecache_sim {

  /**
   * A vm that will be placed in the future
   */
  struct FutureVM {
    uint32_t cores;
    uint32_t memory;
    uint32_t len;
    bool multiTh;
    std::string usage;
    std::string application;
  };

  struct Application {
    int cost_get;
    int cost_set;

    // parameters for connection to cache
    std::string addr;
    int port;
    
    GeneratorConfiguration genCfg;
  };

  /**
   * Main class of a simulation
   * Instantiate the datacenters, the PMs, and VMs
   */
  class Simulation {
  private:

    // The dc managed by the sim
    virt::Datacenter _dc;

    // Vms to spawn in the future
    std::map <uint32_t, std::vector <FutureVM> > _futureVMs;

    // Map of applications by their name
    std::unordered_map<std::string, Application> _applications;
    
    std::map <std::string, virt::TraceCreator> _tcs;

  public:

    Simulation ();

    /**
     * @params:
     *    - cfgPath: the path of the configuration file
     */
    void configure (const std::string & cfgPath, const std::string & outPath);

    /**
     * Execute the simulation
     */
    void execute ();

    /**
     * Clear the simulator
     */
    void dispose ();

    /**
     * this-> dispose ();
     */
    ~Simulation ();

  private:

    void spawnFutureVMs (uint32_t index);

    void configureGlobals (const rd_utils::utils::config::ConfigNode & cfg);

    void configureTraceCreators (const rd_utils::utils::config::ConfigNode & cfg);

    void configureVMs (const rd_utils::utils::config::ConfigNode & cfg);

    void configureCaches (const rd_utils::utils::config::ConfigNode & cfg);

  };
}
