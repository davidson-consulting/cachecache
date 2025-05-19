#pragma once

#include <string>
#include <rd_utils/_.hh>

namespace deployer {

    class Deployment;
    class Machine;
    class Application;

    class Gatling {
    private:

        struct ScenarioInfo {
            uint32_t nbUsers;
            uint32_t duration;
            std::string scenario;
        };

    private:

        Deployment * _context;

    private:

        // The machine running the gatling
        std::string _host;

        // The application stressed by the gatling
        std::shared_ptr <Application> _app;

        // The source file of the gatling to run
        std::string _srcFile;

    public :

        /**
         * Configure a gatling runner for the app
         */
        Gatling (Deployment * context, std::shared_ptr <Application> app, const std::string & name);

        /**
         * Configure the gatling
         */
        void configure (const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * Download the gatling result files
         */
        void downloadResults (const std::string & resultDir);

    private:

        /**
         * Create the scenario source file
         */
        std::string createScenarioSource (const std::vector <ScenarioInfo> & scenario);

    };


}
