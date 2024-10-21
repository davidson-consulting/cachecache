#define LOG_LEVEL 4
#include "simulation.hh"

#include <rd_utils/concurrency/timer.hh>
#include <chrono>

#include <application.hh>

namespace cachecache_sim {

  Simulation::Simulation () {}

  void Simulation::configure (const std::string & cfgPath, const std::string & outPath) {
    auto cfg = rd_utils::utils::toml::parseFile (cfgPath);
    if (cfg-> contains ("global")) {
      this-> configureGlobals ((*cfg)["global"]);
    }

    this-> _dc.configure ((*cfg) ["datacenter"], outPath);
    this-> configureTraceCreators ((*cfg)["modals"]);
    this-> configureVMs ((*cfg)["vms"]);
  }

  void Simulation::execute () {
    uint32_t index = 0;
    rd_utils::concurrency::timer t;
    while (true) {
      this-> spawnFutureVMs (index);
      this-> _dc.next ();

      if (this-> _futureVMs.empty ()) {
        if (this-> _dc.isFinished ()) break;
      }

      index += 1;
      if (index % 100 == 0) LOG_INFO ("I = ", index);
    }

    LOG_INFO ("Simulation took ", index, " iterations");
    LOG_INFO ("For an actual time of ", t.time_since_start ());
  }

  void Simulation::spawnFutureVMs (uint32_t index) {
    auto fnd = this-> _futureVMs.find (index);
    if (fnd != this-> _futureVMs.end ()) {
      for (auto & v : fnd-> second) {
        auto tc = this-> _tcs.find (v.usage);
        LOG_INFO ("Spawn a new VM at t=", index, ", (", v.cores, ",", v.memory, ")-> ", v.len);
        this-> _dc.spawn (v.cores, v.memory, tc-> second, v.len, v.multiTh, std::move(Application()));
      }

      this-> _futureVMs.erase (index);
    }
  }

  void Simulation::dispose () {
    this-> _dc.dispose ();
    this-> _futureVMs.clear ();
  }

  Simulation::~Simulation () {
    this-> dispose ();
  }

  void Simulation::configureTraceCreators (const rd_utils::utils::config::ConfigNode & cfg) {
    auto modals = (const rd_utils::utils::config::Dict*) (&cfg);
    for (auto & it : modals-> getKeys ()) {
      auto & modal = (*modals)[it];
      std::vector <std::pair <float, float> > mods;
      std::vector <float> weights;
      auto usages = (const rd_utils::utils::config::Array*)(&modal ["usages"]);
      for (uint32_t i = 0 ; i < usages-> getLen () ; i++) {
        mods.push_back (std::pair <float, float> ((*usages)[i]["average"].getF (),
                                                  (*usages)[i]["variance"].getF ()));
        weights.push_back ((*usages)[i]["weight"].getF ());
      }

      auto freqA = modal ["frequency"][0].getI ();
      auto freqB = modal ["frequency"][1].getI ();

      this-> _tcs.emplace (it, virt::TraceCreator (mods, weights, {freqA, freqB}));
    }
  }

  void Simulation::configureVMs (const rd_utils::utils::config::ConfigNode & cfg) {
    auto vms = (const rd_utils::utils::config::Dict*) (&cfg);

    for (auto & i : vms-> getKeys ()) {
      uint32_t cores = (*vms)[i]["nb_cores"].getI ();
      uint32_t memory = (*vms)[i]["memory"].getI ();
      uint32_t nbInst = (*vms)[i]["nb_instances"].getI ();
      uint32_t startA = (*vms)[i]["start"][0].getI ();
      uint32_t startB = (*vms)[i]["start"][1].getI ();
      uint32_t lenA = (*vms)[i]["length"][0].getI ();
      uint32_t lenB = (*vms)[i]["length"][1].getI ();
      bool multiTh = (*vms)[i].getOr ("multi_threads", false);
      std::string modal = (*vms)[i]["usage"].getStr ();

      if (this-> _tcs.find (modal) == this-> _tcs.end ()) {
        throw std::runtime_error ("Unkwon usage model : " + modal);
      }

      auto startEngine = std::uniform_real_distribution <float> (startA, startB);
      auto lenEngine = std::uniform_real_distribution <float> (lenA, lenB);
      for (uint32_t j = 0 ; j < nbInst ; j++) {
        uint32_t start = startEngine (config::GlobalConfiguration::instance ().getRandomEngine ());
        uint32_t len = lenEngine (config::GlobalConfiguration::instance ().getRandomEngine ());

        this-> _futureVMs [start].push_back (FutureVM {.cores = cores,
                                                       .memory = memory,
                                                       .len = len,
                                                       .multiTh = multiTh,
                                                       .usage = modal});
      }
    }
  }

  void Simulation::configureGlobals (const rd_utils::utils::config::ConfigNode & cfg) {
    config::GlobalConfiguration::instance ()
      .htSlowDown (cfg.getOr ("ht_slowdown", 1.9))
      .randomSeed (cfg.getOr ("seed", std::chrono::system_clock::now().time_since_epoch().count()));
  }


}

