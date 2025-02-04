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
        this-> deployDB ();
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
        try {
            auto host = this-> _context-> getCluster ()-> get (this-> _db.host);
            auto path = utils::join_path (host-> getHomeDir (), "run/apps/" + this-> _name);
            LOG_INFO ("Closing mysql docker image on : ", this-> _db.host);
            host-> run ("cd " + path + "; docker-compose down")-> wait ();
        } catch (...) {}

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

            // Read db configuration
            this-> _db.host = cfg ["db"]["host"].getStr ();
            this-> _db.name = cfg ["db"]["name"].getStr ();
            this-> _db.port = cfg ["db"].getOr ("port", (int64_t) this-> _db.port);
            this-> _db.base = cfg ["db"].getOr ("base", this-> _db.base);
            if (this-> _db.base != "") {
                this-> _db.base = utils::join_path (this-> _context-> getConfigDirPath (), this-> _db.base);
                if (!utils::file_exists (this-> _db.base)) {
                    throw std::runtime_error ("Base sql file '" + this-> _db.base + "' not found");
                }
            }
            this-> _context-> getCluster ()-> get (this-> _db.host)-> addFlag ("db");

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
            
            LOG_INFO ("Register app database for ", this-> _name, " on host ", this-> _db.host);

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

                                this-> _services [host].emplace (name, ServiceReplications {.nb = nb, .cache = ch});
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


    void Application::deployDB () {
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
            "      - " + std::to_string (this-> _db.port) + ":3306\n"
            "    command: mysqld --lower_case_table_names=1 --skip-ssl --character_set_server=utf8mb4 --explicit_defaults_for_timestamp\n";

        auto host = this-> _context-> getCluster ()-> get (this-> _db.host);
        auto path = this-> appPath (host);

        auto script =
            "cd " + path + "\n"
            "docker-compose up -d";

        host-> putFromStr (cmp, utils::join_path (path, "compose.yaml"));
        host-> runScript (script)-> wait ();
        concurrency::timer t;

        LOG_INFO ("Deploying DB on '", this-> _db.host);
        t.sleep (1);

        if (this-> _context-> installDB ()) {
            host-> runAndWait ("mysql -e \"drop database if exists socialNet;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _db.port), 50, 0.2);
            host-> runAndWait ("mysql -e \"create database if not exists socialNet;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _db.port), 50, 0.2);
            LOG_INFO ("Upload sql file ", this-> _db.base, " to '", this-> _db.host, "':", utils::join_path (path, "dump.sql"));
            host-> put (this-> _db.base, utils::join_path (path, "dump.sql"));

            host-> runAndWait ("cd " + path + "; mysql -e \"use socialNet; source dump.sql;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _db.port), 50, 0.2);
            LOG_INFO ("Recreated db on ", this-> _db.host, " (took : ", t.time_since_start (), "s)");
        } else {
            host-> runAndWait ("mysql -e \"create database if not exists socialNet;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _db.port), 50, 0.2);
            LOG_INFO ("Created db on ", this-> _db.host, " (took : ", t.time_since_start (), "s)");
        }
    }

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

        if (this-> _context-> installDB ()) { // clean timelines
            host-> run ("cd " + path + "; rm -rf .home")-> wait ();
            host-> run ("cd " + path + "; rm -rf .post")-> wait ();
            LOG_INFO ("Cleaned user timelines");
        }

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
        bool needDb = false;
        uint32_t nbAll = 0;

        auto services = std::make_shared <config::Dict> ();
        for (auto & it : this-> _services [hostName]) {
            auto serConf = std::make_shared <config::Dict> ();
            if (it.second.cache.name != "") {
                caches.emplace (it.second.cache.name, it.second.cache.supervisor);
                serConf-> insert ("cache", it.second.cache.name);
            }

            if (it.first != "compose" && it.first != "text") {
                serConf-> insert ("db", "mysql");
                needDb = true;
            }

            serConf-> insert ("nb", (int64_t) it.second.nb);
            nbAll += it.second.nb;

            services-> insert (it.first, serConf);
        }
        result-> insert ("services", services);

        if (needDb) {
            auto dbs = std::make_shared <config::Dict> ();
            auto db = std::make_shared <config::Dict> ();
            db-> insert ("addr", this-> _context-> getCluster ()-> get (this-> _db.host)-> getIp ());
            db-> insert ("name", this-> _db.name);
            db-> insert ("port", (int64_t) this-> _db.port);
            db-> insert ("user", "root");
            db-> insert ("pass", "");

            dbs-> insert ("mysql", db);
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
