#include "analyser.hh"
#include <rd_utils/foreign/CLI11.hh>
#include "../tex/_.hh"


using namespace rd_utils;
using namespace rd_utils::utils;

namespace analyser {

    Analyser::Analyser () {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Analyser::configure (int argc, char ** argv) {
        this-> parseCmdOptions (argc, argv);

        try {
            this-> _minTimestamp = -1;
            this-> _runConfig = toml::parseFile (utils::join_path (this-> _traceDir, "config.toml"));

            this-> _cluster.configure (this-> _traceDir, (*this-> _runConfig) ["machines"]);
            if (this-> _minTimestamp > this-> _cluster.getMinTimestamp ()) {
                this-> _minTimestamp = this-> _cluster.getMinTimestamp ();
            }

            this-> configureCaches ((*this-> _runConfig));
            if (utils::file_exists (utils::join_path (this-> _traceDir, "gatling.log"))) {
                this-> configureGatling (utils::join_path (this-> _traceDir, "gatling.log"));
            }
        } catch (const std::runtime_error & err) {
            LOG_ERROR ("Failed to configure the analyse ", err.what ());
            ::exit (-1);
        }
    }

    void Analyser::configureCaches (const config::ConfigNode & cfg) {
        if (cfg.contains ("caches")) {
            match (cfg ["caches"]) {
                of (config::Dict, dc) {
                    for (auto & name : dc-> getKeys ()) {
                        auto host = (*dc)[name]["host"].getStr ();
                        // auto hostname = cfg ["machines"][host]["hostname"].getStr ();

                        this-> _caches [name].configure (this-> _minTimestamp, this-> _traceDir, host, name);
                    }
                } fo;
            }
        }
    }

    void Analyser::configureGatling (const std::string & logPath) {
        this-> _gatling = std::make_shared <Gatling> ();
        this-> _gatling-> configure (logPath);
    }

    void Analyser::parseCmdOptions (int argc, char ** argv) {
        CLI::App app;
        this-> _traceDir = "";
        this-> _outDir = "./output";

        app.add_option ("-t,--trace-dir", this-> _traceDir, "the directory containing the run traces to analyse")-> required ();
        app.add_option ("-o,--output-dir", this-> _outDir, "the directory in which results will be exported (default = ./output)");

        try {
            app.parse (argc, argv);
        } catch (const CLI::ParseError &e) {
            ::exit (app.exit (e));
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          EXECUTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Analyser::execute () {
        utils::create_directory (this-> _outDir, true);
        auto doc = std::make_shared <tex::Beamer> ("cluster");
        this-> _cluster.execute (this-> _minTimestamp, doc);

        for (auto & it : this-> _caches) {
            it.second.execute (doc);
        }

        if (this-> _gatling != nullptr) {
            this-> _gatling-> execute (this-> _minTimestamp, doc);
        }

        doc-> write (this-> _outDir);
    }


}
