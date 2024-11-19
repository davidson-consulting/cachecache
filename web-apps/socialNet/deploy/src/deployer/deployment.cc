#include "deployment.hh"
#include <rd_utils/foreign/CLI11.hh>
#include <csignal>

#include "cluster.hh"
#include "app.hh"
#include "cache.hh"
#include "installer.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

deployer::Deployment * __GLOBAL_DEPLOY__ = nullptr;

void ctrlCHandler (int signum) {
  LOG_INFO ("Signal ", strsignal(signum), " received");
  if (__GLOBAL_DEPLOY__ != nullptr) {
    __GLOBAL_DEPLOY__-> kill ();
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
            auto cfg = toml::parseFile (this-> _hostFile);

            this-> configureCluster (*cfg);
            this-> configureCaches (*cfg);
            this-> configureApplications (*cfg);

        } catch (const std::runtime_error & err) {
            LOG_ERROR ("Failed to configure deployement ", err.what ());
            ::exit (-1);
        }
    }

    void Deployment::parseCmdOptions (int argc, char ** argv) {
        CLI::App app;
        this-> _installNodes = false;
        this-> _installDB = false;

        app.add_option("-c,--config-path", this-> _hostFile, "the path of the configuration file");
        app.add_flag ("--install", this-> _installNodes, "install applications on hosts");
        app.add_flag ("--install-db", this-> _installDB, "reset mysql volumes, and install base bdd (also reset user timelines)");

        try {
            app.parse (argc, argv);
        } catch (const CLI::ParseError &e) {
            ::exit (app.exit (e));
        }
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

    void Deployment::configureApplications (const config::ConfigNode & cfg) {
        // No application to deploy ?
        if (!cfg.contains ("apps")) return;

        match (cfg ["apps"]) {
            of (config::Dict, dc) {
                for (auto & name : dc-> getKeys ()) {
                    auto a = std::make_shared <deployer::Application> (this, name);
                    a-> configure ((*dc)[name]);
                    this-> _apps.emplace (name, a);
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
        __GLOBAL_DEPLOY__ = this;
        ::signal (SIGINT, &ctrlCHandler);
        ::signal (SIGTERM, &ctrlCHandler);
        ::signal (SIGKILL, &ctrlCHandler);


        if (this-> _installNodes) {
            Installer i (*this-> _cluster);
            i.execute ();
        }

        for (auto & it : this-> _caches) {
            it.second-> start ();
        }

        for (auto & it : this-> _apps) {
            it.second-> start ();
        }
    }

    void Deployment::join () {
        for (auto & it : this-> _apps) {
            it.second-> join ();
        }

        for (auto & it : this-> _caches) {
            it.second-> join ();
        }
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
    
}
