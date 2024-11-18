#pragma once

#include <rd_utils/_.hh>

namespace deployer {

    class Cluster;
    class Application {
    private:

        struct ServiceReplications {
            uint32_t nb;
            std::string cacheName;
        };

        struct ServiceDeployement {
            uint32_t nbAll; // the number of services in total on the node
            std::map <std::string, ServiceReplications> services;
        };

        struct CacheDeployement {
            rd_utils::utils::MemorySize size;
            std::map <std::string, rd_utils::utils::MemorySize> entities;
        };

        Cluster * _context;

        // The name of the application
        std::string _name;

    private:

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ======================================          MAIN          ======================================
         * ====================================================================================================
         * ====================================================================================================
         */

        // The host deploying the registry
        std::string _registryHost;

        // The host deploying the BDD
        std::string _bddHost;

        // The host deploying the front
        std::string _frontHost;

        // The name of the cache used by the front (can be "" if no cache)
        std::string _frontCache;

    private:

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * =====================================          CACHE          ======================================
         * ====================================================================================================
         * ====================================================================================================
         */

        // The host for the cache (can be "" if no cache)
        std::string _cacheHost;

        // The cache deployement configuration (if cacheHost != "")
        CacheDeployement _ch;

    private:

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          SERVICES          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        // The list of host deploying instance of compose service
        std::map <std::string, ServiceDeployement> _services;

    private:

        // The temporary directory in which temp files are exported
        std::string _tmpDir;

        // The running cmds
        std::vector <std::shared_ptr <rd_utils::concurrency::SSHProcess> > _running;

    private:

        // The port of the registry of the app
        uint32_t _regPort;

        // The port of the cache instances
        std::map <std::string, uint32_t> _cachePorts;

        // The port of the front of the app
        uint32_t _frontPort;

    public:

        /**
         * Create an application on a specific cluster
         * @params:
         *    - name: the name of the application
         */
        Application (Cluster & c, const std::string & name);

        /**
         * Configure the applciation
         */
        void configure (const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * Start the remote application
         */
        void start ();

        /**
         * Join the remote application
         * @info: unless there is a crash, this should be infinite
         */
        void join ();

    private:

        /**
         * Deploy the mysql instance
         */
        void deployBdd () ;

        /**
         * Create a registry configuration file
         */
        std::shared_ptr <rd_utils::utils::config::ConfigNode> createRegistryConfig ();

        /**
         * Deploy the registry on a given host
         * @params:
         *    - hostName: the name of the machine
         *    - cfg: the configuration file used
         * @returns: the port on which the registry is running
         */
        uint32_t deployRegistry (std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg);

        /**
         * Create the cache supervisor configuration file
         */
        std::shared_ptr <rd_utils::utils::config::ConfigNode> createCacheSupervisorConfig (const std::string & hostName);

        /**
         * Deploy a cache supervisor on a host
         * @returns: the port on which the supervisor is running
         */
        uint32_t deploySupervisor (const std::string & hostName, std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg);

        /**
         * Create the cache instances configuration file
         * @params:
         *    - supervisorPort: the port on which the supervisor is running
         */
        std::shared_ptr <rd_utils::utils::config::ConfigNode> createCacheInstancesConfig (const std::string & hostName, uint32_t supervisorPort);

        /**
         * Deploy cache instances on a given host
         * @returns: the list of port of the different cache entities
         */
        std::map <std::string, uint32_t> deployCacheInstances (const std::string & hostName, std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg);

        /**
         * Create the configuration file for services on a given host
         */
        std::shared_ptr <rd_utils::utils::config::ConfigNode> createServiceConfig (const std::string & hostName, uint32_t registryPort, std::map <std::string, uint32_t> cachePort);

        /**
         * Deploy the services on a given host
         * @params:
         *   - hostName: the name of the host
         *   - cfg: the configuration file
         *
         */
        void deployServices (const std::string & hostName, std::shared_ptr <rd_utils::utils::config::ConfigNode> cfg);

        /**
         * Create the configuration for the front end
         * @returns: the configuration file
         */
        std::shared_ptr <rd_utils::utils::config::ConfigNode> createFrontConfig (uint32_t registryPort, std::map <std::string, uint32_t> cachePort);

        /**
         * Deploy the front end
         */
        void deployFront (std::shared_ptr <rd_utils::utils::config::ConfigNode> & cfg);


    private:

        /**
         * Read the configuration of front/reg and BDD
         */
        void readMainConfiguration (const rd_utils::utils::config::Dict & cfg);

        /**
         * Read the configuration of the micro services
         */
        void readServiceConfiguration (const rd_utils::utils::config::Dict & cfg);

        /**
         * Read the configuration of a single service
         * @params:
         *   - name: the name of the service (e.g. compose, post, etc.)
         */
        void readSingleServiceConfiguration (const std::string & name, const rd_utils::utils::config::Dict & cfg);

        /**
         * Read the configuration of the cache (if any)
         */
        void readCacheConfiguration (const rd_utils::utils::config::Dict & cfg);

        /**
         * Read the configuration of the bdd
         */
        void readBddConfiguration (const rd_utils::utils::config::Dict & cfg);
    };


}
