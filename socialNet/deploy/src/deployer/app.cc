#include "app.hh"
#include "deployment.hh"
#include "cluster.hh"
#include "cache.hh"
#include "db.hh"

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
        this-> deployRegistry ();
        for (auto & it : this-> _services) {
            this-> deployServices (it.first);
        }

        this-> deployFront ();
    }

    void Application::kill () {
        LOG_INFO ("Kill app ", this-> _name);

        for (auto & it : this-> _running) {
            it-> dispose ();
        }

        for (auto & it : this-> _killing) {
            auto mch = this-> _context-> getCluster ()-> get (it.first);
            for (auto & jt : it.second) {
                LOG_INFO ("Run kill cmd : [", jt, "] on host ", it.first);
                mch-> run (jt)-> wait ();
            }
        }

        this-> _killing.clear ();
    }

    void Application::clean () {
        for (auto & it : this-> _services) {
            try {
                auto host = this-> _context-> getCluster ()-> get (it.first);
                auto path = utils::join_path (host-> getHomeDir (), "run/apps/" + this-> _name);
                LOG_INFO ("Remove app directory '" + this-> _name + "' on : ", it.first);
                host-> run ("rm -rf " + path)-> wait ();
            } catch (...) {}
        }

        try {
            auto host = this-> _context-> getCluster ()-> get (this-> _front.host);
            auto path = utils::join_path (host-> getHomeDir (), "run/apps/" + this-> _name);
            LOG_INFO ("Remove app directory '" + this-> _name + "' on : ", this-> _front.host);
            host-> run ("rm -rf " + path)-> wait ();
        } catch (...) {}

        try {
            auto host = this-> _context-> getCluster ()-> get (this-> _registryHost);
            auto path = utils::join_path (host-> getHomeDir (), "run/apps/" + this-> _name);
            LOG_INFO ("Remove app directory '" + this-> _name + "' on : ", this-> _registryHost);
            host-> run ("rm -rf " + path)-> wait ();
        } catch (...) {}
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
            // Read registry configuration
            this-> _registryHost = cfg ["registry"]["host"].getStr ();
            this-> _context-> getCluster ()-> get (this-> _registryHost)-> addFlag ("socialNet");

            // Read front configuration
            this-> _front.host = cfg ["front"]["host"].getStr ();
            this-> _front.cache = this-> findCacheFromName (cfg ["front"].getOr ("cache", ""));
            this-> _front.threads = cfg ["front"].getOr ("threads", -1);
            this-> _front.port = cfg ["front"].getOr ("port", (int64_t) this-> _front.port);
            this-> _context-> getCluster ()-> get (this-> _front.host)-> addFlag ("socialNet");

            LOG_INFO ("Register app registry for ", this-> _name, " on host ", this-> _registryHost);
            if (this-> _front.cache.name == "") {
                LOG_INFO ("Register app front for ", this-> _name, " on host ", this-> _front.host, " without cache");
            } else {
                LOG_INFO ("Register app front for ", this-> _name, " on host ", this-> _front.host, " with cache ", this-> _front.cache.supervisor-> getName (), ".", this-> _front.cache.name);
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
                                auto dbN = (*dc).getOr ("db", "");

                                this-> _context-> getCluster ()-> get (host)-> addFlag ("social");
                                auto ch = this-> findCacheFromName (cache);
                                auto db = this-> findDBFromName (dbN);

                                if (ch.name == "") {
                                    LOG_INFO ("Register ", nb, " service replicates '", name, "' on host ", host, " for '", this-> _name, "' with no cache");
                                } else {
                                    LOG_INFO ("Register ", nb, " service replicates '", name, "' on host '", host, "' for ", this-> _name, " with cache ", ch.supervisor-> getName (), ".", ch.name);
                                }

                                this-> _services [host].emplace (name, ServiceReplications {.nb = nb, .cache = ch, .db = db});
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


    Application::DBRef Application::findDBFromName (const std::string & name) {
        // No cache
        if (name == "") return DBRef {.name = "", .supervisor = nullptr};

        auto inst = this-> _context-> getDB (name);
        return DBRef {.name = name, .supervisor = inst};
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CREATE CONFIGS          =================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Application::deployRegistry () {
        auto host = this-> _context-> getCluster ()-> get (this-> _registryHost);
        auto cfg = this-> createRegistryConfig (host);
        auto path = this-> appPath (host);

        auto script =
            "cd " + path + "\n"
            "rm registry.out.txt\n"
            "./reg -c ./res/registry.toml >> registry.out.txt 2>&1  &\n"
            "echo -e $! > pidof_registry\n";

        host-> putFromStr (cfg, utils::join_path (path, "res/registry.toml"));
        LOG_INFO ("Launching app '", this-> _name , "' registry on ", this-> _registryHost);

        this-> _running.push_back (host-> runScript (script));
        LOG_INFO ("Waiting for the registry to be avaible");

        auto pidStr = host-> getToStr (utils::join_path (path, "pidof_registry"), 10, 0.1);
        auto pid = std::strtoull (pidStr.c_str (), NULL, 0);

        host-> run ("cgclassify -g cpu,memory:apps/" + this-> _name + " " + pidStr)-> wait ();

        auto portStr = host-> getToStr (utils::join_path (path, "registry_port"), 10, 0.1);
        this-> _registryPort = std::strtoull (portStr.c_str (), NULL, 0);

        LOG_INFO ("App '", this-> _name, "' registry  is running on ", this-> _registryHost, " on port = ", this-> _registryPort, ", and pid = ", pid);
        this-> _killing [this-> _registryHost].push_back ("kill -2 " + std::to_string (pid));
    }

    void Application::deployServices (const std::string & hostName) {
        auto host = this-> _context-> getCluster ()-> get (hostName);
        auto cfg = this-> createServiceConfig (hostName, host);
        auto path = this-> appPath (host);

        auto script =
            "cd " + path + "\n"
            "rm services.out.txt\n"
            "./services -c ./res/services.toml >> services.out.txt 2>&1  &\n"
            "echo -e $! > pidof_services\n";

        host-> putFromStr (cfg, utils::join_path (path, "res/services.toml"));
        LOG_INFO ("Launching app '", this-> _name , "' services on ", hostName);

        this-> _running.push_back (host-> runScript (script));
        LOG_INFO ("Waiting for the services to be avaible");

        auto pidStr = host-> getToStr (utils::join_path (path, "pidof_services"), 10, 0.1);
        auto pid = std::strtoull (pidStr.c_str (), NULL, 0);

        host-> run ("cgclassify -g cpu,memory:apps/" + this-> _name + " " + pidStr)-> wait ();

        LOG_INFO ("App '", this-> _name, "' services are running on ", hostName, " pid = ", pid);
        this-> _killing [hostName].push_back ("kill -2 " + std::to_string (pid));
    }
    
    void Application::deployFront () {
        auto host = this-> _context-> getCluster ()-> get (this-> _front.host);
        auto cfg = this-> createFrontConfig (host);
        auto path = this-> appPath (host);

        auto script =
            "cd " + path + "\n"
            "rm front.out.txt\n"
            "./front -c ./res/front.toml >> front.out.txt 2>&1  &\n"
            "echo -e $! > pidof_front\n";

        host-> putFromStr (cfg, utils::join_path (path, "res/front.toml"));
        LOG_INFO ("Launching app '", this-> _name , "' front on ", this-> _front.host);

        this-> _running.push_back (host-> runScript (script));
        LOG_INFO ("Waiting for the front to be avaible");

        auto pidStr = host-> getToStr (utils::join_path (path, "pidof_front"), 10, 0.1);
        auto pid = std::strtoull (pidStr.c_str (), NULL, 0);

        host-> run ("cgclassify -g cpu,memory:apps/" + this-> _name + " " + pidStr)-> wait ();

        LOG_INFO ("App '", this-> _name, "' front is running on ", this-> _front.host, " pid = ", pid);
        this-> _killing [this-> _front.host].push_back ("kill -2 " + std::to_string (pid));
    }


    std::string Application::appPath (std::shared_ptr <Machine> host) {
        auto path = utils::join_path (host-> getHomeDir (), "run/apps/" + this-> _name);
        if (this-> _hostHasDir.find (host-> getName ()) == this-> _hostHasDir.end ()) {
            LOG_INFO ("Create app directory for ", host-> getName ());
            host-> run ("mkdir -p " + path)-> wait ();
            host-> run ("mkdir -p " + utils::join_path (path, "res"))-> wait ();
            host-> run ("cp " + utils::join_path (host-> getHomeDir (), "execs/socialNet/front") + " " + path)-> wait ();
            host-> run ("cp " + utils::join_path (host-> getHomeDir (), "execs/socialNet/services") + " " + path)-> wait ();
            host-> run ("cp " + utils::join_path (host-> getHomeDir (), "execs/socialNet/reg") + " " + path)-> wait ();

            host-> run ("cgcreate -g cpu,memory:apps")-> wait ();
            host-> run ("cgcreate -g cpu,memory:apps/" + this-> _name)-> wait ();

            this-> _hostHasDir.emplace (host-> getName ());
        }

        return path;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ================================          CONFIG CREATION          =================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::string Application::createRegistryConfig (std::shared_ptr <Machine> host) {
        auto cfg = config::Dict ()
            .insert ("port", (int64_t) 0)
            .insert ("addr", "0.0.0.0")
            .insert ("iface", host-> getIface ());

        return toml::dump (cfg);
    }

    std::string Application::createFrontConfig (std::shared_ptr <Machine> host) {
        auto regHost = this-> _context-> getCluster ()-> get (this-> _registryHost);
        auto result = std::make_shared <config::Dict> ();
        auto sys = std::make_shared <config::Dict> ();
        sys-> insert ("port", (int64_t) 0);
        sys-> insert ("iface", host-> getIface ());
        sys-> insert ("threads", (int64_t) 1);
        result-> insert ("sys", sys);

        auto server = std::make_shared <config::Dict> ();
        server-> insert ("port", (int64_t) this-> _front.port);
        server-> insert ("iface", host-> getIface ());
        server-> insert ("threads", (int64_t) this-> _front.threads);
        result-> insert ("server", server);

        auto reg = std::make_shared <config::Dict> ();
        reg-> insert ("name", "registry");
        reg-> insert ("addr", regHost-> getIp ());
        reg-> insert ("port", (int64_t) this-> _registryPort);
        result-> insert ("registry", reg);

        if (this-> _front.cache.name != "") {
            auto cache = std::make_shared <config::Dict> ();
            cache-> insert ("addr", this-> _front.cache.supervisor-> getHost ()-> getIp ());
            cache-> insert ("port", (int64_t) this-> _front.cache.supervisor-> getPort (this-> _front.cache.name));
            result-> insert ("cache", cache);
        }

        return toml::dump (*result);
    }

    std::string Application::createServiceConfig (const std::string & hostName, std::shared_ptr <Machine> host) {
        auto regHost = this-> _context-> getCluster ()-> get (this-> _registryHost);

        auto result = std::make_shared <config::Dict> ();

        std::map <std::string, std::shared_ptr <Cache> > caches;
        std::vector <std::string> needDbs;
        uint32_t nbAll = 0;

        auto services = std::make_shared <config::Dict> ();
        for (auto & it : this-> _services [hostName]) {
            auto serConf = std::make_shared <config::Dict> ();
            if (it.second.cache.name != "") {
                caches.emplace (it.second.cache.name, it.second.cache.supervisor);
                serConf-> insert ("cache", it.second.cache.name);
            }

            if (it.first != "compose" && it.first != "text") {
                serConf-> insert ("db", it.second.db.name);
                needDbs.push_back (it.second.db.name);
            }

            serConf-> insert ("nb", (int64_t) it.second.nb);
            nbAll += it.second.nb;

            services-> insert (it.first, serConf);
        }
        result-> insert ("services", services);

        if (needDbs.size () != 0) {
            auto dbs = std::make_shared <config::Dict> ();
            for (auto & it : needDbs) {
                auto deployDB = this-> _context-> getDB (it);
                auto db = deployDB-> createConfig ();
                dbs-> insert (it, db);
            }

            result-> insert ("db", dbs);
        }

        if (caches.size () > 0) {
            auto cacheConfigs = std::make_shared <config::Dict> ();
            for (auto & ch : caches) {
                auto port = ch.second-> getPort (ch.first);
                auto chInst = std::make_shared<config::Dict> ();
                chInst-> insert ("addr", ch.second-> getHost ()-> getIp ());
                chInst-> insert ("port", (int64_t) port);

                cacheConfigs-> insert (ch.first, chInst);
            }

            result-> insert ("cache", cacheConfigs);
        }

        if (nbAll == 0) nbAll = 1;

        auto sys = std::make_shared <config::Dict> ();
        sys-> insert ("port", (int64_t) 0);
        sys-> insert ("iface", host-> getIface ());
        sys-> insert ("threads", (int64_t) nbAll);
        result-> insert ("sys", sys);

        auto auth = std::make_shared <config::Dict> ();
        auth-> insert ("secret", "password");
        auth-> insert ("issuer", "auth0");
        result-> insert ("auth", auth);

        auto reg = std::make_shared <config::Dict> ();
        reg-> insert ("name", "registry");
        reg-> insert ("addr", regHost-> getIp ());
        reg-> insert ("port", (int64_t) this-> _registryPort);
        result-> insert ("registry", reg);

        return toml::dump (*result);
    }

}
