#pragma once

#include <rd_utils/_.hh>

namespace deployer {

        class Deployment;
        class Cache;

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

                struct ServiceDeployement {
                        uint32_t nbAll; // the number of services in total on the node
                        std::map <std::string, ServiceReplications> services;
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

                // The host deploying the BDD
                std::string _dbHost;

                // The host deploying the front
                std::string _frontHost;

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
                std::map <std::string, ServiceDeployement> _services;

        private:

                // The temporary directory in which temp files are exported
                std::string _tmpDir;

                // The running cmds
                std::vector <std::shared_ptr <rd_utils::concurrency::SSHProcess> > _running;

                // Cmd to run when killing the app
                std::map <std::string, std::vector <std::string> > _killing;

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
                 * Join the remote application
                 * @info: unless there is a crash, this should be infinite
                 */
                void join ();

                /**
                 * Kill every processes launched by the app
                 */
                void kill ();

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
                 * @returns: the cache reference from ch ("supervisor.entity")
                 * @throws: if cache was not found
                 * @info: if ch == "", returns an empty reference
                 */
                CacheRef findCacheFromName (const std::string & ch);

        };


}
