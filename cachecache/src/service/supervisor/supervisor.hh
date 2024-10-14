#pragma once 
#include <rd_utils/foreign/CLI11.hh>
#include <rd_utils/utils/_.hh>
#include <rd_utils/concurrency/_.hh>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <service/cachecache.hh>
#include <service/clock/clock.hh>
#include <service/generator.hh>
#include <service/metrics/metrics.hh>
#include <service/market.hh>

namespace cachecache {
    class Supervisor {
        public:
            Supervisor();
            ~Supervisor();

            void configure(int argc, char ** argv);
            
            /**
             * Start the required caches 
            */
            void run();

            std::unordered_map<std::string, Cachecache>& getRunningCaches();

        private:
            Metrics _metrics;
            std::unique_ptr<Market> _market;

            // map between a name and its cache
            std::unordered_map<std::string, Cachecache> _caches; 
            std::unordered_map<std::string, Clock> _clocks;
            std::unordered_map<std::string, Generator> _generators;
            std::unordered_map<std::string, std::shared_ptr<bool>> _generator_finished;

            std::vector<rd_utils::concurrency::Thread> _threads;


            std::string _cfgPath;
            
            /*
            *   CLI
            */
            // The number of parameters passed to the program
            int _argc;
            // The parameters passed to the program
            char ** _argv;

            // The app option parser
            CLI::App _app;
            CLI::Option * _cfgPathOpt;

            size_t _cachesize;

            void initAppOptions();
            void configure(const std::shared_ptr<rd_utils::utils::config::ConfigNode> & config);
    };
}
