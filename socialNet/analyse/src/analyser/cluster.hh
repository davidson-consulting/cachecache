#pragma once

#include "machine.hh"
#include <map>

#include <rd_utils/_.hh>
#include "../tex/_.hh"

namespace analyser {

    class Cluster {
    private:

        // The machines in the cluster
        std::map <std::string, Machine> _machines;

        // The first timestamp of the traces of the machines
        uint64_t _minTimestamp;

    public:

        Cluster ();

        /**
         * Configure the cluster
         */
        void configure (const std::string & traceDir, const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * Execute the analyse of the cluster
         * @params:
         *    - globalMinTimestamp: the global minimal timestamp of the analyse (among caches, and cluster)
         *    - outDir: the directory in which outputing the results
         */
        void execute (uint64_t globalMinTimestamp, std::shared_ptr <tex::Beamer> doc);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GET/SET          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Move the minimal timestamp of the cluster
         * @info: insert zeros at the beginning of the traces of each machines if this-> _minTimestamp > timestamp, do nothing otherwise
         */
        void setMinTimestamp (uint64_t timestamp);

        /**
         * @returns: the min timestamp of the cluster
         */
        uint64_t getMinTimestamp () const;

        /**
         * @returns: the number of machines in the cluster
         */
        uint64_t getNbMachines () const;

    };


}
