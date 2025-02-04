#pragma once

#include <string>
#include <rd_utils/_.hh>

#include "cluster.hh"
#include "cache.hh"

namespace analyser {

    class Analyser {
    private:

        // The path to the directory containing the traces
        std::string _traceDir;

        // The path to the directory containing the results
        std::string _outDir;

        // The config read from the traces
        std::shared_ptr <rd_utils::utils::config::ConfigNode> _runConfig;

    private:

        // The instant of the first point among all loaded traces
        uint64_t _minTimestamp;

        // The cluster of this analyse
        Cluster _cluster;

        // The list of cache executed during the deployement
        std::map <std::string, Cache> _caches;

    public:

        Analyser ();

        /**
         * Configure the analyser
         */
        void configure (int argc, char ** argv);

        /**
         * Execute the analyser and generate the result files from the traces
         */
        void execute ();


    private:

        /**
         * Parse the command line options
         */
        void parseCmdOptions (int argc, char ** argv);

        /**
         * Configure the caches
         */
        void configureCaches (const rd_utils::utils::config::ConfigNode & cfg);

    };


}
