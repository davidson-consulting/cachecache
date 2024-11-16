#pragma once

#include <rd_utils/_.hh>
#include <string>

namespace deployer {

    class Cluster;
    class Installer {
    private :

        // The path of the working directory of the installer
        std::string _wd;

        // The list of script to execute on each nodes
        std::map <std::string, std::vector <std::string> > _scripts;

        // The context of the installation
        Cluster * _context;

        // Task pool to run installation scripts in parallel
        std::vector <rd_utils::concurrency::Thread> _threads;

        rd_utils::concurrency::mutex _m;

    public:

        /**
         * @params: The working directory
         */
        Installer (Cluster & c, const std::string & wd);

        /**
         * Execute the installation process on every nodes of the cluster according to their respective configuration
         */
        void execute (const rd_utils::utils::config::ConfigNode & cfg);

    private:

        /**
         * @returns: the path to the installation scripts according to the configuration file
         */
        std::vector <std::string> getInstallScripts (const std::string & nodeName, const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * Run the script on the machine
         */
        void runScripts (rd_utils::concurrency::Thread, std::string mc);
    } ;


}
