#pragma once

#include <rd_utils/_.hh>

namespace deployer {

        class Deployment;
        class Cache;
        class Machine;

        class Application {
        private:

                struct CacheRef {
                        std::string name;
                        std::shared_ptr <Cache> supervisor;
                };

                struct ServiceReplications {
                        uint32_t nb;
                        CacheRef cache;
                };

        private:

                Deployment * _context;

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

                // The host deploying the DB
                std::string _dbHost;

                // The name of the db used by the app
                std::string _dbName;

                // The port of the db
                uint32_t _dbPort = 3306;

                // The host deploying the front
                std::string _frontHost;

                // The number of threads of the front
                int32_t _frontThreads = 0;

                // The port of the front app
                uint32_t _frontPort = 8080;

                // The name of the cache used by the front (can be "" if no cache)
                CacheRef _frontCache;

        private:

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ====================================          SERVICES          ====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                // The list of host deploying instance of compose service
                std::map <std::string, std::map <std::string, ServiceReplications> > _services;

                // List of host whose app path was created
                std::set <std::string> _hostHasDir;

        private:

                // The temporary directory in which temp files are exported
                std::string _tmpDir;

                // The remote commands launched by the app
                std::vector <std::shared_ptr <rd_utils::concurrency::SSHProcess> > _running;

                // Cmd to run when killing the app
                std::map <std::string, std::vector <std::string> > _killing;

        private:

                // The port of the registry of the app
                uint32_t _registryPort;

                // The port of the cache instances
                std::map <std::string, uint32_t> _cachePorts;

        public:

                /**
                 * Create an application on a specific cluster
                 * @params:
                 *    - name: the name of the application
                 */
                Application (Deployment * c, const std::string & name);

                /**
                 * Configure the applciation
                 */
                void configure (const rd_utils::utils::config::ConfigNode & cfg);

                /**
                 * Start the remote application
                 */
                void start ();

                /**
                 * Kill every processes launched by the app
                 */
                void kill ();

        private:

                /**
                 * Deploy the mysql instance
                 */
                void deployDB () ;

                /**
                 * Deploy the registry of the app
                 */
                void deployRegistry ();

                /**
                 * Deploy the services on a given host
                 * @params:
                 *   - hostName: the name of the host
                 *   - cfg: the configuration file
                 *
                 */
                void deployServices (const std::string & hostName);

                /**
                 * Deploy the front end
                 */
                void deployFront ();

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ================================          CONFIG CREATION          =================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Create a registry configuration file
                 */
                std::string createRegistryConfig (std::shared_ptr <Machine> host);


                /**
                 * Create the configuration file for services on a given host
                 */
                std::string createServiceConfig (const std::string & hostName, std::shared_ptr <Machine> host);

                /**
                 * Create the configuration for the front end
                 * @returns: the configuration file
                 */
                std::string createFrontConfig (std::shared_ptr <Machine> host);

        private:

                /**
                 * Read the configuration of front/reg and DB
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
                 * @returns: the cache reference from ch ("supervisor.entity")
                 * @throws: if cache was not found
                 * @info: if ch == "", returns an empty reference
                 */
                CacheRef findCacheFromName (const std::string & ch);

                /**
                 * @returns: the path to the bin files on the machine host, and create them if they don't exist
                 */
                std::string appPath (std::shared_ptr <Machine> host);

        };


}
