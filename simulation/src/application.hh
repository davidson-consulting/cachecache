#pragma once

#include <vcpu_sim/virtual/application.hh>

namespace cachecache_sim {
  class Application: public vcpu_sim::virt::Application {
    public:
      Application() = default;

      /**
       * Execute "nbFlops" instructions in the
       * application
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
  };
};
