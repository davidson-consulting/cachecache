#pragma once

#include <rd_utils/_.hh>
#include <map>
#include <memory>

namespace deployer {

    class Cluster;
    class Application;
    class Cache;

    class Deployment {
    private:

        // The cluster on which the deployment is made
        std::shared_ptr<Cluster> _cluster;

        // The list of application to deploy
        std::map <std::string, std::shared_ptr <Application> > _apps;

        // The list of cache to deploy
        std::map <std::string, std::shared_ptr <Cache> > _caches;

    private:

        // The file containing the configuration
        std::string _hostFile;

        // Install packages on nodes (otherwise assume everything is already installed)
        bool _installNodes;

        // Reinstall the DB on the nodes (otherwise assume everything is already installed)
        bool _installDB;

        // To wait indefinitely
        rd_utils::concurrency::semaphore _sem;

    public:

        Deployment ();

        /**
         * Configure the deployement
         */
        void configure (int argc, char ** argv);

        /**
         * Start the deployement
         */
        void start ();

        /**
         * Wait for the deployment to join (infinite unless there is a crash)
         */
        void join ();

        /**
         * Kill the deployement
         */
        void kill ();

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GETTERS          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * @returns: the cluster of the deployment
         */
        std::shared_ptr <Cluster> getCluster ();

        /**
         * @returns: the cache deployment
         * @throws: if the cache does not exist
         */
        std::shared_ptr <Cache> getCache (const std::string & name);

    private:

        /**
         * Parse the command line options
         * @warning: exit the application if command line are wrong
         */
        void parseCmdOptions (int argc, char ** argv);

        /**
         * Configure the machines of the deployement
         */
        void configureCluster (const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * Configure the cached to deploy
         */
        void configureCaches (const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * Configure the applications to deploy
         */
        void configureApplications (const rd_utils::utils::config::ConfigNode & cfg);

    };

}
