#pragma once

#include <map>
#include <string>

#include "machine.hh"

namespace deployer {

    class Cluster {
    private :

        // The list of machines in the cluster
        std::map <std::string, std::shared_ptr <Machine> > _machines;

    public:

        /**
         * Create an empty cluster
         */
        Cluster ();

        /**
         * Create a cluster of nodes from a config file
         * @throws: if the configuration file is malformed
         */
        void configure (const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * @returns: a machine from the cluster using its name
         * @throws: if the machine does not exist
         */
        Machine& operator[] (const std::string & name);

        /**
         * @returns: a machine from the cluster using its name
         * @throws: if the machine does not exist
         */
        std::shared_ptr <Machine> get (const std::string & name);

        /**
         * @returns: the name of the machines in the cluster
         */
        std::vector <std::string> machines () const;

        /**
         * Clean the machines temporary files
         */
        void clean ();
    };

}
