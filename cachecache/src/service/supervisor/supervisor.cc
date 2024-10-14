#include <service/supervisor/supervisor.hh>
#include <unistd.h>
#include <chrono>
#include "cachelib/common/PeriodicWorker.h"
#include "cachelib/allocator/memory/Slab.h"

using namespace cachecache;
using namespace rd_utils::utils;
using namespace rd_utils::concurrency;

Supervisor::Supervisor(){}
Supervisor::~Supervisor(){}

std::unordered_map<std::string, Cachecache>& Supervisor::getRunningCaches() {
    return this->_caches;
}

void Supervisor::run() {
    // start each generator in its own thread 
    for(auto & generator: this->_generators) {
        this->_threads.push_back(spawn(&generator.second, &Generator::run));
    }

    //facebook::cachelib::util::startPeriodicWorker("market", this->_market, std::chrono::milliseconds(500));
    /*MarketConfig marketCfg = {
        this->_cachesize, // global memory pool 
        0.75, // percentage of memory used before triggering an increment
        0.1, // percentage of increment 
        0.3, // percentage of memory used before triggering a decrement 
        0.1, // percentage of decrement 
        3 * facebook::cachelib::Slab::kSize,
    };

    this->_market->configure(marketCfg, &this->_metrics);
    for (auto & [name, cache]: this->_caches) {
        this->_market->register_cache(name, &cache);
    }*/

    sleep(1);

    while (true) {
        bool all_finished = true;
        for (auto & [cache_name, finished]: this->_generator_finished) {
            if (!(*finished)) all_finished = false;
        }

        if (all_finished) break;

        for (auto & [cache_name, cache]: this->_caches) {
            cache.clean();
        }

        sleep(3);
    }

    for(auto & thread: this->_threads) {
        join(thread);
    }

    //facebook::cachelib::util::stopPeriodicWorker("market", this->_market);

    sleep(1);
}

void Supervisor::configure(int argc, char ** argv) {
    this-> _argc = argc;
    this-> _argv = argv;
    this-> initAppOptions();

    if(this-> _cfgPathOpt-> count() == 0) {
        std::cout << "Needs configuration file" << std::endl;
        exit(-1);
    }

    std::string configPath;

    try {
        configPath = rd_utils::utils::get_absolute_path(this-> _cfgPath);
    } catch (const rd_utils::utils::FileError& e) {
        LOG_ERROR("No config file found at : ", this-> _cfgPath);
        exit(-1);
    }

    this-> configure(rd_utils::utils::toml::parseFile(configPath));
}

void Supervisor::initAppOptions() {
    this-> _cfgPathOpt = this-> _app.add_option("-c,--config-path", this-> _cfgPath, "the path of the configuration file");
    try {
        this-> _app.parse(this-> _argc, this-> _argv);
    } catch (const CLI::ParseError &e) {
        exit(this-> _app.exit(e));
    }
}

void Supervisor::configure(const std::shared_ptr<rd_utils::utils::config::ConfigNode> & config) {
    if ((*config).contains("main")) {
        auto & main_config = (*config)["main"];
        this->_cachesize = main_config["cache_size"].getI() * 1024 * 1024;
    }
    this->_metrics.configure("/tmp");


    if((*config).contains("caches")) {
        match((*config)["caches"]) {
            of(config::Dict, caches_config) {
                for(auto & c: caches_config->getKeys()) {
                    auto & cache_config = (*caches_config)[c];
                    
                    auto & name = cache_config["name"].getStr();
                    size_t requested = cache_config["requested"].getI() * 1024 * 1024;
                    double p0 = 0.75; //cache_config["p0"].getF();// / 100.f;
                    double p1 = 0.95; //cache_config["p1"].getF();// / 100.f;
                    double p2 = 0.9999; //cache_config["p2"].getF();// / 100.f;

                    XLOG(INFO, "CONFIG ", p0, " ", p1, " ", p2);

                    Clock clock;
                    this->_clocks.insert_or_assign(name, std::move(clock));

                    //Cachecache cache;
                    //cache.configure(size, p0, p1, p2, &this->_clocks.at(name));
                    //this->_caches.insert_or_assign(name, std::move(cache));
                    this->_caches[name].configure(name, this->_cachesize, requested, p0, p1, p2, &this->_clocks.at(name), &this->_metrics); 
                }
            } elfo {
                LOG_ERROR("Caches declaration should be a TOML dict");
                exit(-1);
            }
        }
    }

    if ((*config).contains("generators")) {
        match ((*config)["generators"]) {
            of (config::Dict, generators_config) {
                for (auto &g: generators_config->getKeys()) {
                    auto & generator_config = (*generators_config)[g];

                    auto & target = generator_config["target"].getStr();
                    auto & traces = generator_config["traces"].getStr();
                    int nb_seconds = generator_config["nb_seconds"].getI();
                    int frequency = generator_config["frequency"].getI();

                    if (this->_caches.find(target) == this->_caches.end()) {
                        LOG_ERROR("Generator wants to target non existing cache named ", target);
                        exit(-1);
                    }

                    auto finished = std::make_shared<bool>(false);
                    this->_generator_finished.insert_or_assign(target, finished);

                    Generator generator;
                    generator.configure(traces, nb_seconds, frequency, &this->_caches.at(target), &this->_clocks.at(target), finished);
                    this->_generators.insert_or_assign(target, std::move(generator));
                }
            } elfo {
                LOG_ERROR("Generators declaration should be a TOML dict");
                exit(-1);
            }
        }
    }
}
