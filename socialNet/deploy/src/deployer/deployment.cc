#include "deployment.hh"
#include <rd_utils/foreign/CLI11.hh>
#include <csignal>

#include "cluster.hh"
#include "app.hh"
#include "cache.hh"
#include "db.hh"
#include "installer.hh"
#include "gatling.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

deployer::Deployment * __GLOBAL_DEPLOY__ = nullptr;

void ctrlCHandler (int signum) {
    LOG_INFO ("Signal ", strsignal(signum), " received");
    if (__GLOBAL_DEPLOY__ != nullptr) {
        __GLOBAL_DEPLOY__-> kill ();
        __GLOBAL_DEPLOY__-> downloadResults ();
        __GLOBAL_DEPLOY__ = nullptr;
    }

  ::exit (0);
}

namespace deployer {

    Deployment::Deployment () {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Deployment::configure (int argc, char ** argv) {
        this-> parseCmdOptions (argc, argv);

        try {
            this-> _cfg = toml::parseFile (this-> _hostFile);

            this-> configureCluster (*this-> _cfg);
            this-> configureDBs (rd_utils::utils::parent_directory (this-> _hostFile), *this-> _cfg);
            this-> configureCaches (*this-> _cfg);
            this-> configureApplications (*this-> _cfg);

        } catch (const std::runtime_error & err) {
            LOG_ERROR ("Failed to configure deployement ", err.what ());
            ::exit (-1);
        }
    }

    void Deployment::parseCmdOptions (int argc, char ** argv) {
        CLI::App app;
        this-> _installNodes = false;
        this-> _installDB = false;
        this-> _onlyClean = false;
        this-> _resultDir = "./results";
        this-> _hostFile = "./hosts.toml";

        app.add_option("-c,--config-path", this-> _hostFile, "the path of the configuration file (default = ./hosts.toml)");
        app.add_option ("-o,--output-dir", this-> _resultDir, "the directory in which results will be exported (default = ./results)");
        app.add_flag ("--install", this-> _installNodes, "install applications on hosts");
        app.add_flag ("--install-db", this-> _installDB, "reset mysql volumes, and install base bdd (also reset user timelines)");
        app.add_flag ("--clean", this-> _onlyClean, "clean temporary files created on remote nodes (does not consider other flags, and does not launch anything)");

        try {
            app.parse (argc, argv);
        } catch (const CLI::ParseError &e) {
            ::exit (app.exit (e));
        }

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        std::stringstream ss;
        ss << std::put_time(&tm, "%d-%m-%Y.%H-%M-%S");

        this-> _resultDir = utils::join_path (this-> _resultDir, ss.str ());
    }

    void Deployment::configureCluster (const config::ConfigNode & cfg) {
        this-> _cluster = std::make_shared <Cluster> ();
        this-> _cluster-> configure (cfg);
    }

    void Deployment::configureCaches (const config::ConfigNode & cfg) {
        // No cache to configure, and therefore no cache to deploy
        if (!cfg.contains ("caches")) return ;

        match (cfg ["caches"]) {
            of (config::Dict, dc) {
                for (auto & name : dc-> getKeys ()) {
                    auto c = std::make_shared <deployer::Cache> (this, name);
                    c-> configure ((*dc)[name]);
                    this-> _caches.emplace (name, c);
                }
            } elfo {
                throw std::runtime_error ("Cache configuration malformed requires a dictionnary");
            }
        }
    }

    void Deployment::configureDBs (const std::string & cwd, const config::ConfigNode & cfg) {
        if (!cfg.contains ("dbs")) return;

        match (cfg ["dbs"]) {
            of (config::Dict, dc) {
                for (auto & name : dc-> getKeys ()) {
                    auto d = std::make_shared <deployer::DB> (this, name);
                    d-> configure (cwd, (*dc)[name]);
                    this-> _dbs.emplace (name, d);
                }
            } elfo {
                throw std::runtime_error ("Databases configuration malformed requires a dictionnary");
            }
        }
    }

    void Deployment::configureApplications (const config::ConfigNode & cfg) {
        // No application to deploy ?
        if (!cfg.contains ("apps")) return;

        match (cfg ["apps"]) {
            of (config::Dict, dc) {
                for (auto & name : dc-> getKeys ()) {
                    auto a = std::make_shared <deployer::Application> (this, name);
                    a-> configure ((*dc)[name]);
                    this-> _apps.emplace (name, a);

                    if ((*dc)[name].contains ("gatling")) {
                        auto gname = (*dc)[name]["gatling"].getStr ();
                        if (!cfg.contains ("gatlings") || !cfg ["gatlings"].contains (gname)) {
                            throw std::runtime_error ("Malformed gatling configuration for app : " + name);
                        }

                        // auto gatlingConfig = cfg ["gatlings"].get (gname);
                        // auto g = std::make_shared <deployer::Gatling> (this, a, name);
                        // g-> configure (*gatlingConfig);
                        // this-> _gatling.emplace (name, g);
                    }
                }
            } elfo {
                throw std::runtime_error ("Apps configuration malformed requires a dictionnary");
            }
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          EXECUTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Deployment::start () {
        if (this-> _onlyClean) {
            this-> clean ();
            return;
        }

        __GLOBAL_DEPLOY__ = this;
        ::signal (SIGINT, &ctrlCHandler);
        ::signal (SIGTERM, &ctrlCHandler);
        ::signal (SIGKILL, &ctrlCHandler);

        try {
            if (this-> _installNodes) {
                Installer i (*this-> _cluster);
                i.execute ();
            }

            this-> _cluster-> prepareVJoule ();
            for (auto & it : this-> _dbs) {
                if (this-> _installDB) {
                    it.second-> restoreBase ();
                }
                it.second-> start ();
            }


            for (auto & it : this-> _caches) {
                it.second-> start ();
            }

            for (auto & it : this-> _apps) {
                it.second-> start ();
            }
        } catch (const std::runtime_error & err) {
            LOG_ERROR ("Failed to start deployement : ", err.what ());
            this-> kill ();
            ::exit (-1);
        }
    }

    void Deployment::join () {
        this-> _sem.wait ();
    }

    void Deployment::downloadResults () {
        utils::create_directory (this-> _resultDir, true);

        // Write the configuration that was used in this run
        utils::write_file (utils::join_path (this-> _resultDir, "config.toml"), toml::dump (*this-> _cfg));

        try {
            this-> _cluster-> downloadVJouleTraces (this-> _resultDir);
        } catch (...) {}

        for (auto & it : this-> _caches) {
            try {
                it.second-> downloadResults (this-> _resultDir);
            } catch (...) {}
        }

        auto latest = utils::join_path (utils::parent_directory (this-> _resultDir), "latest");
        try {
            utils::remove (latest);
        } catch (...) {}

        utils::create_symlink (latest, utils::get_filename (this-> _resultDir));

        // for (auto & it : this-> _gatling) {
        //     try {
        //         it.second-> downloadResults (this-> _resultDir);
        //     } catch (...) {}
        // }
    }

    void Deployment::kill () {
        LOG_INFO ("Kill deployement");
        for (auto & it : this-> _apps) {
            it.second-> kill ();
        }

        for (auto & it : this-> _caches) {
            it.second-> kill ();
        }
    }

    void Deployment::clean () {
        LOG_INFO ("Clean deployement");
        for (auto & it : this-> _apps) {
            it.second-> clean ();
        }

        for (auto & it : this-> _dbs) {
            it.second-> clean ();
        }

        for (auto & it : this-> _caches) {
            it.second-> clean ();
        }

        this-> _cluster-> clean ();
        this-> _sem.post (); // cleaning is directly joined
    }


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::shared_ptr <Cluster> Deployment::getCluster () {
        return this-> _cluster;
    }

    std::shared_ptr <Cache> Deployment::getCache (const std::string & name) {
        auto it = this-> _caches.find (name);
        if (it == this-> _caches.end ()) {
            throw std::runtime_error ("No cache named : " + name);
        }

        return it-> second;
    }

    std::shared_ptr <DB> Deployment::getDB (const std::string & name) {
        auto it = this-> _dbs.find (name);
        if (it == this-> _dbs.end ()) {
            throw std::runtime_error ("No db named : " + name);
        }

        return it-> second;
    }

    bool Deployment::installDB () const {
        return this-> _installDB;
    }

    std::string Deployment::getConfigDirPath () const {
        return utils::parent_directory (this-> _hostFile);
    }
    
}
