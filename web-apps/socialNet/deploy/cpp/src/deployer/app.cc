#include "app.hh"
#include "cluster.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace deployer {

    Application::Application (Cluster & c, const std::string & name) :
        _context (&c)
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

    void Application::join () {}

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
                this-> readCacheConfiguration (*dc);
                this-> readMainConfiguration (*dc);
                this-> readServiceConfiguration (*dc);
                this-> readBddConfiguration (*dc);
            } elfo {
                throw std::runtime_error ("Malformed configuration for " + this-> _name);
            }
        }
    }

    void Application::readMainConfiguration (const config::Dict & cfg) {
        try {
            this-> _registryHost = cfg ["registry"]["host"].getStr ();
            this-> _context-> get (this-> _registryHost)-> addFlag ("socialNet");

            this-> _bddHost = cfg ["bdd"]["host"].getStr ();
            this-> _context-> get (this-> _bddHost)-> addFlag ("bdd");

            this-> _frontHost = cfg ["front"]["host"].getStr ();
            this-> _context-> get (this-> _frontHost)-> addFlag ("socialNet");
            if (cfg ["front"].contains ("cache")) {
                auto name = cfg ["front"]["cache"].getStr ();
                if (this-> _ch.entities.find (name) == this-> _ch.entities.end ()) {
                    throw std::runtime_error ("No cache named : " + name + " for front");
                }

                this-> _frontCache = name;
            }

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
                                this-> _context-> get (host)-> addFlag ("social");
                                if (cache != "" && this-> _ch.entities.find (cache) == this-> _ch.entities.end ()) {
                                    throw std::runtime_error ("No cache named : " + cache + " for " + name);
                                }

                                auto fnd = this-> _services.find (host);
                                if (fnd == this-> _services.end ()) {
                                    ServiceDeployement n = ServiceDeployement {.nbAll = nb, .services = {}};
                                    n.services.emplace (name, ServiceReplications {.nb = nb, .cacheName = cache});
                                } else {
                                    fnd-> second.nbAll += nb;
                                    fnd-> second.services.emplace (name, ServiceReplications {.nb = nb, .cacheName = cache});
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

    void Application::readCacheConfiguration (const config::Dict & cfg) {
        if (cfg.contains ("cache")) {
            try {
                this-> _cacheHost = cfg ["cache"]["host"].getStr ();
                auto unit = cfg ["cache"]["unit"].getStr ();
                this-> _ch.size = MemorySize::unit (cfg ["cache"]["size"].getI (), unit);
                this-> _context-> get (this-> _cacheHost)-> addFlag ("cache");

                match (cfg ["cache"]["entities"]) {
                    of (config::Dict, en) {
                        for (auto & it : en-> getKeys ()) {
                            this-> _ch.entities.emplace (it, MemorySize::unit ((*en)[it].getI (), unit));
                        }
                    } fo;
                }
            } catch (const std::runtime_error & err) {
                LOG_ERROR ("Failed to load cache configuration : ", err.what ());
                throw std::runtime_error ("Malformed cache configuration for " + this-> _name);
            }
        }
    }

    void Application::readBddConfiguration (const config::Dict & cfg) {
        try {
            this-> _bddHost = cfg ["bdd"]["host"].getStr ();
            this-> _context-> get (this-> _bddHost)-> addFlag ("bdd");
        } catch (const std::runtime_error & err) {
            LOG_ERROR ("Failed to read BDD config", err.what ());
            throw std::runtime_error ("Malformed cache configuration for " + this-> _name);
        }
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

        this-> _context-> get (this-> _bddHost)-> putFromStr (cmp, "compose.yaml");
        auto p = this-> _context-> get (this-> _bddHost)-> run ("docker-compose up -d");
        p-> wait ();

        LOG_INFO (p-> stdout ());
    }

    std::shared_ptr <config::ConfigNode> Application::createRegistryConfig () {
        auto cfg = std::make_shared <config::Dict> ();
        cfg-> insert ("port", (int64_t) 0);
        cfg-> insert ("addr", "0.0.0.0");
        cfg-> insert ("iface", this-> _context-> get (this-> _registryHost)-> getIface ());

        return cfg;
    }

    uint32_t Application::deployRegistry (std::shared_ptr <config::ConfigNode> cfg) {
        return 0;
    }


}
