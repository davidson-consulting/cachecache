#pragma once

#include <rd_utils/_.hh>
#include <string>

namespace deployer {

        class Cluster;
        class Machine;

        class Installer {
        private :

                // The context of the installation
                Cluster * _context;

                // Task pool to run installation scripts in parallel
                std::vector <rd_utils::concurrency::Thread> _threads;

                rd_utils::concurrency::mutex _m;

        public:

                /**
                 * @params: The working directory
                 */
                Installer (Cluster & c);

                /**
                 * Execute the installation process on every nodes of the cluster according to their respective configuration
                 */
                void execute ();

        private:

                /**
                 * Install on a specific node
                 * @params:
                 *   - mc: the machine id
                 */
                void installOnNode (rd_utils::concurrency::Thread, std::string mc);

                /**
                 * Run apt commands
                 */
                void runAptInstalls (std::shared_ptr <Machine> m);

                /**
                 * Install libhttpserver
                 */
                void runHttpInstall (std::shared_ptr <Machine> m);

                /**
                 * Install rd_utils
                 */
                void runRdUtilsInstall (std::shared_ptr <Machine> m);

                /**
                 * Install socialNet
                 */
                void runSocialNetInstall (std::shared_ptr <Machine> m);

                /**
                 * Install cache
                 */
                void runCacheInstall (std::shared_ptr <Machine> m, const std::string & version);

                /**
                 * Install gatling
                 */
                void runGatlingInstall (std::shared_ptr <Machine> m);

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * =====================================          VJOULE          =====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                /**
                 * Configure the vjoule service and cgroups
                 */
                void runConfigureVjoule (std::shared_ptr <Machine> m);

                /**
                 * Create the vjoule toml configuration file
                 */
                std::string createVjouleConfig (std::shared_ptr <Machine> m) const;


        };


}
