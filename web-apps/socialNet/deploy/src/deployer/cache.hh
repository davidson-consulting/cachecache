#pragma once

#include <rd_utils/_.hh>

namespace deployer {

    class Deployment;
    class Machine;

    /**
     * A cache supervisor/instances deployer
     */
    class Cache {
    private:

        struct CacheDeployement {

        };

    private:

        Deployment * _context;

        // The name of the cache
        std::string _name;

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

        // The size of the cache supervised
        rd_utils::utils::MemorySize _size;

        // The size of the entities to deploy
        std::map <std::string, rd_utils::utils::MemorySize> _entities;

    private:

        // The temporary directory in which temp files are exported
        std::string _tmpDir;

        // The running cmds
        std::vector <std::shared_ptr <rd_utils::concurrency::SSHProcess> > _running;

        // Cmd to run when killing the app
        std::vector <std::string>  _killing;

        // The port on which the supervisor is deployed
        uint32_t _superPort;

        // The port on which entities are deployed
        std::map <std::string, uint32_t> _entitiesPort;

    public:

        Cache (Deployment * context, const std::string & name);

        /**
         * Configure the cache to deploy
         */
        void configure (const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * Start the cache supervisor and instances
         */
        void start ();

        /**
         * Wait for the cache instance to finish @infinite
         */
        void join ();

        /**
         * Kill every processes started by the cache
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
         * @returns: true if the cache has an entity named name
         */
        bool hasEntity (const std::string & name) const;

        /**
         * @returns: the name of the cache
         */
        const std::string & getName () const;

    private:

        /**
         * Read the configuration of the cache
         */
        void readCacheConfiguration (const rd_utils::utils::config::Dict & cfg);

        /**
         * Deploy a cache supervisor on a host
         */
        void deploySupervisor ();

        /**
         * Deploy cache instances on a given host
         */
        void deployCacheEntity (const std::string & entity);

        /**
         * Create the cache supervisor configuration file
         */
        std::string createCacheSupervisorConfig (std::shared_ptr <Machine> host);

        /**
         * Create the cache instances configuration file
         * @params:
         *    - supervisorPort: the port on which the supervisor is running
         */
        std::string createCacheEntityConfig (uint32_t supervisorPort);

    };

}
