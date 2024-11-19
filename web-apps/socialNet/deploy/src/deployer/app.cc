#include "app.hh"
#include "deployment.hh"
#include "cluster.hh"
#include "cache.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace deployer {

    Application::Application (Deployment * c, const std::string & name) :
        _context (c)
        , _name (name)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          START          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Application::start () {
        this-> deployBdd ();
        auto cfg = this-> createRegistryConfig ();
    }

    void Application::join () {
        for (auto & it : this-> _running) {
            it-> wait ();
        }

        this-> _running.clear ();
        this-> kill ();
    }

    void Application::kill () {
        LOG_INFO ("Kill app ", this-> _name);
        for (auto & it : this-> _running) {
            it-> dispose ();
        }

        this-> _running.clear ();
        for (auto & it : this-> _killing) {
            auto mch = this-> _context-> getCluster ()-> get (it.first);
            for (auto & jt : it.second) {
                mch-> run (jt)-> wait ();
            }
        }

        this-> _killing.clear ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Application::configure (const rd_utils::utils::config::ConfigNode & cfg) {
        match (cfg) {
            of (config::Dict, dc) {
                this-> readMainConfiguration (*dc);
                this-> readServiceConfiguration (*dc);
            } elfo {
                throw std::runtime_error ("Malformed configuration for " + this-> _name);
            }
        }
    }

    void Application::readMainConfiguration (const config::Dict & cfg) {
        try {
            this-> _registryHost = cfg ["registry"]["host"].getStr ();
            this-> _context-> getCluster ()-> get (this-> _registryHost)-> addFlag ("socialNet");

            this-> _dbHost = cfg ["db"]["host"].getStr ();
            this-> _context-> getCluster ()-> get (this-> _dbHost)-> addFlag ("db");

            this-> _frontHost = cfg ["front"]["host"].getStr ();
            this-> _context-> getCluster ()-> get (this-> _frontHost)-> addFlag ("socialNet");
            this-> _frontCache = this-> findCacheFromName (cfg ["front"].getOr ("cache", ""));

            LOG_INFO ("Register app registry for ", this-> _name, " on host ", this-> _registryHost);
            if (this-> _frontCache.name == "") {
                LOG_INFO ("Register app front for ", this-> _name, " on host ", this-> _frontHost, " without cache");
            } else {
                LOG_INFO ("Register app front for ", this-> _name, " on host ", this-> _frontHost, " with cache ", this-> _frontCache.supervisor-> getName (), ".", this-> _frontCache.name);
            }
            
            LOG_INFO ("Register app database for ", this-> _name, " on host ", this-> _dbHost);

        } catch (const std::runtime_error & err) {
            LOG_ERROR ("Failed to read app configuration : ", err.what ());
            throw std::runtime_error ("Malformed main configuration for " + this-> _name);
        }
    }

    void Application::readServiceConfiguration (const config::Dict & cfg) {
        this-> readSingleServiceConfiguration ("compose", cfg);
        this-> readSingleServiceConfiguration ("post", cfg);
        this-> readSingleServiceConfiguration ("social", cfg);
        this-> readSingleServiceConfiguration ("text", cfg);
        this-> readSingleServiceConfiguration ("user", cfg);
        this-> readSingleServiceConfiguration ("short", cfg);
        this-> readSingleServiceConfiguration ("timeline", cfg);
    }

    void Application::readSingleServiceConfiguration (const std::string & name, const config::Dict & cfg) {
        try {
            match (cfg [name]) {
                of (config::Array, arr) {
                    for (uint32_t i = 0 ; i < arr-> getLen () ; i++) {
                        match ((*arr)[i]) {
                            of (config::Dict, dc) {
                                auto host = (*dc)["host"].getStr ();
                                uint32_t nb = (*dc)["nb"].getI ();
                                auto cache = (*dc).getOr ("cache", "");
                                this-> _context-> getCluster ()-> get (host)-> addFlag ("social");
                                auto ch = this-> findCacheFromName (cache);

                                if (ch.name == "") {
                                    LOG_INFO ("Register ", nb, " service replicates '", name, "' on host ", host, " for '", this-> _name, "' with no cache");
                                } else {
                                    LOG_INFO ("Register ", nb, " service replicates '", name, "' on host '", host, "' for ", this-> _name, " with cache ", ch.supervisor-> getName (), ".", ch.name);
                                }

                                auto fnd = this-> _services.find (host);
                                if (fnd == this-> _services.end ()) {
                                    ServiceDeployement n = ServiceDeployement {.nbAll = nb, .services = {}};
                                    n.services.emplace (name, ServiceReplications {.nb = nb, .cache = ch});
                                } else {
                                    fnd-> second.nbAll += nb;
                                    fnd-> second.services.emplace (name, ServiceReplications {.nb = nb, .cache = ch});
                                }
                            } elfo {
                                throw std::runtime_error ("Malformed service configuration for " + this-> _name);
                            }
                        }
                    }
                } elfo {
                    throw std::runtime_error ("Malformed service configuration for " + this-> _name);
                }
            }
        } catch (const std::runtime_error & err) {
            LOG_ERROR ("Malformed config : ", err.what ());
            throw std::runtime_error ("Malformed service configuration for " + this-> _name);
        }
    }

    Application::CacheRef Application::findCacheFromName (const std::string & name) {
        // No cache
        if (name == "") return CacheRef {.name = "", .supervisor = nullptr};

        auto fnd = name.find (".");
        if (fnd == std::string::npos) {
            throw std::runtime_error ("Malformed cache name : " + name + " (expected ${cacheName}.${entityName})");
        }

        auto cacheName = name.substr (0, fnd);
        auto entityName = name.substr (fnd + 1);

        auto cacheInst = this-> _context-> getCache (cacheName);
        if (!cacheInst-> hasEntity (entityName)) {
            throw std::runtime_error ("Malformed cache name : " + name + " (cache instance " + cacheName + " does not have an entity named " + entityName + ")");
        }

        return CacheRef {.name = entityName, .supervisor = cacheInst};
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CREATE CONFIGS          =================================
     * ====================================================================================================
     * ====================================================================================================
     */


    void Application::deployBdd () {
        auto cmp =
            "version: '3.8'\n"
            "services:\n"
            "  mysql:\n"
            "    image: mysql:8.0.30\n"
            "    volumes:\n"
            "      - ./config/mysql:/etc/mysql/conf.d\n"
            "    environment:\n"
            "      - MYSQL_ALLOW_EMPTY_PASSWORD=yes\n"
            "    ports:\n"
            "      - 3306:3306\n"
            "    command: mysqld --lower_case_table_names=1 --skip-ssl --character_set_server=utf8mb4 --explicit_defaults_for_timestamp\n";

        this-> _context-> getCluster ()-> get (this-> _dbHost)-> putFromStr (cmp, "compose.yaml");
        auto p = this-> _context-> getCluster ()-> get (this-> _dbHost)-> run ("docker-compose up -d");
        p-> wait ();

        LOG_INFO (p-> stdout ());
    }

    std::shared_ptr <config::ConfigNode> Application::createRegistryConfig () {
        auto cfg = std::make_shared <config::Dict> ();
        cfg-> insert ("port", (int64_t) 0);
        cfg-> insert ("addr", "0.0.0.0");
        cfg-> insert ("iface", this-> _context-> getCluster ()-> get (this-> _registryHost)-> getIface ());

        return cfg;
    }

    uint32_t Application::deployRegistry (std::shared_ptr <config::ConfigNode> cfg) {
        return 0;
    }


}
