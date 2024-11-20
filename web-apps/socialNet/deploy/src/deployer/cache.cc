#include "cache.hh"
#include "deployment.hh"
#include "cluster.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace deployer {

    Cache::Cache (Deployment * context, const std::string & name)
        : _context (context)
        , _name (name)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cache::configure (const config::ConfigNode & cfg) {
        match (cfg) {
            of (config::Dict, dc) {
                this-> readCacheConfiguration (*dc);
            } elfo {
                throw std::runtime_error ("Malformed cache (expected dictionnary)");
            }
        }
    }

    void Cache::readCacheConfiguration (const config::Dict & cfg) {
        try {
            this-> _cacheHost = cfg ["host"].getStr ();
            auto unit = cfg ["unit"].getStr ();
            this-> _size = MemorySize::unit (cfg ["size"].getI (), unit);
            this-> _context-> getCluster ()-> get (this-> _cacheHost)-> addFlag ("cache");
            LOG_INFO ("Register cache supervisor ", this-> _name, " of size ", this-> _size.megabytes (), "MB, on host ", this-> _cacheHost);

            match (cfg ["entities"]) {
                of (config::Dict, en) {
                    for (auto & it : en-> getKeys ()) {
                        auto entitySize = MemorySize::unit ((*en)[it]["size"].getI (), unit);
                        uint32_t ttl = (*en)[it].getOr ("ttl", 100);
                        LOG_INFO ("Register cache entity for : ", this-> _name, " ", it, " of size ", entitySize.megabytes (), "MB, with TTL = ", ttl);
                        this-> _entities.emplace (it, EntityInfo {.size = entitySize, .ttl = ttl});
                    }
                } fo;
            }
        } catch (const std::runtime_error & err) {
            LOG_ERROR ("Failed to load cache configuration : ", err.what ());
            throw std::runtime_error ("Malformed cache configuration for " + this-> _name);
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool Cache::hasEntity (const std::string & name) const {
        auto it = this-> _entities.find (name);
        return it != this-> _entities.end ();
    }

    const std::string & Cache::getName () const {
        return this-> _name;
    }

    std::shared_ptr <Machine> Cache::getHost () {
        return this-> _context-> getCluster ()-> get (this-> _cacheHost);
    }

    uint32_t Cache::getPort (const std::string & name) const {
        auto it = this-> _entitiesPort.find (name);
        if (it == this-> _entitiesPort.end ()) throw std::runtime_error ("No cache entity : " + this-> _name + "." + name);

        return it-> second;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          EXECUTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cache::start () {
        LOG_INFO ("Deploying cache : ", this-> _name);
        this-> deploySupervisor ();
        for (auto & it : this-> _entities) {
            this-> deployCacheEntity (it.first);
        }
    }

    void Cache::kill () {
        LOG_INFO ("Kill cache ", this-> _name);
        for (auto & it : this-> _running) {
            it-> dispose ();
        }

        for (auto & it : this-> _killing) {
            LOG_INFO ("Run kill cmd : [", it, "] on host ", this-> _cacheHost);
            this-> _context-> getCluster ()-> get (this-> _cacheHost)-> run (it)-> wait ();
        }
        this-> _killing.clear ();
    }

    void Cache::clean () {
        auto host = this-> _context-> getCluster ()-> get (this-> _cacheHost);
        LOG_INFO ("Remove cache directory  '", this-> _name, "' on : ", this-> _cacheHost);
        auto path = utils::join_path (host-> getHomeDir (), "run/caches/" + this-> _name);
        host-> run ("rm -rf " + path)-> wait ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===============================          SUPERVISOR DEPLOY          ================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cache::deploySupervisor () {
        auto host = this-> _context-> getCluster ()-> get (this-> _cacheHost);
        auto cfg = this-> createCacheSupervisorConfig (host);
        auto path = this-> cachePath ();

        auto script = "cd " + path + "\n"
            "rm super.out.txt\n"
            "./supervisor -c ./res/super.toml >> super.out.txt 2>&1  &\n"
            "echo -e $! > pidof_super\n";

        host-> putFromStr (cfg, utils::join_path (path, "/res/super.toml"));
        LOG_INFO ("Launching cache supervisor on ", this-> _cacheHost);

        this-> _running.push_back (host-> runScript (script));

        LOG_INFO ("Waiting for supervisor to be avaible");
        auto pidStr = host-> getToStr (utils::join_path (path, "pidof_super"), 10, 0.1);
        auto pid = std::strtoull (pidStr.c_str (), NULL, 0);

        auto portStr = host-> getToStr (utils::join_path (path, "super_port"), 10, 0.1);
        this-> _superPort = std::strtoull (portStr.c_str (), NULL, 0);

        LOG_INFO ("Cache supervisor is running on ", this-> _cacheHost, " on port = ", this-> _superPort, ", and pid = ", pid);
        this-> _killing.push_back ("kill -9 " + std::to_string (pid));
    }

    std::string Cache::createCacheSupervisorConfig (std::shared_ptr <Machine> host) {
        auto tracePath = utils::join_path (utils::join_path (host-> getHomeDir (), "traces"), this-> _name + "_traces");

        auto sys = std::make_shared <config::Dict> ();
        sys-> insert ("addr", "0.0.0.0");
        sys-> insert ("port", (int64_t) 0);
        sys-> insert ("freq", (int64_t) 1);
        sys-> insert ("export-traces", tracePath);

        auto cache = std::make_shared <config::Dict> ();
        cache-> insert ("size", (int64_t) this-> _size.kilobytes ());
        cache-> insert ("unit", "KB");

        auto result = config::Dict ()
            .insert ("sys", sys)
            .insert ("cache", cache);

        return toml::dump (result);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          ENTITY DEPLOY          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cache::deployCacheEntity (const std::string & entityName) {
        auto host = this-> _context-> getCluster ()-> get (this-> _cacheHost);
        auto cfg = this-> createCacheEntityConfig (entityName, host);
        auto path = this-> cachePath ();

        auto script = "cd " + path + "\n"
            "rm entity." + entityName + ".out.txt\n"
            "./cache -c ./res/entity." + entityName + ".toml >> entity." + entityName + ".out.txt 2>&1  &\n"
            "echo -e $! > pidof_entity." + entityName + "\n";

        host-> putFromStr (cfg, utils::join_path (path, "res/entity." + entityName + ".toml"));
        LOG_INFO ("Launching cache entity on ", this-> _cacheHost);

        this-> _running.push_back (host-> runScript (script));

        concurrency::timer t;
        LOG_INFO ("Waiting for entity to be avaible");
        t.sleep (1);

        auto pidStr = host-> getToStr (utils::join_path (path, "pidof_entity." + entityName), 20, 0.05);
        auto pid = std::strtoull (pidStr.c_str (), NULL, 0);

        auto portStr = host-> getToStr (utils::join_path (path, "entity_port.0." + entityName), 20, 0.05);
        auto port = std::strtoull (portStr.c_str (), NULL, 0);

        this->_entitiesPort.emplace (entityName, port);
        LOG_INFO ("Cache entity is running on ", this-> _cacheHost, " on port = ", port, ", and pid = ", pid);
        this-> _killing.push_back ("kill -9 " + std::to_string (pid));
    }

    std::string Cache::createCacheEntityConfig (const std::string & entityName, std::shared_ptr <Machine> host) {
        auto tracePath = utils::join_path (utils::join_path (host-> getHomeDir (), "traces"), this-> _name + "." + entityName + "_traces");
        auto sys = std::make_shared <config::Dict> ();
        sys-> insert ("name", entityName);
        sys-> insert ("addr", "0.0.0.0");
        sys-> insert ("port", (int64_t) 0);
        sys-> insert ("instances", (int64_t) 1);
        sys-> insert ("export-traces", tracePath);

        auto super = std::make_shared <config::Dict> ();
        super-> insert ("addr", "127.0.0.1");
        super-> insert ("port", (int64_t) this-> _superPort);

        auto service = std::make_shared <config::Dict> ();
        service-> insert ("addr", "0.0.0.0");
        service-> insert ("port", (int64_t) 0);

        auto cache = std::make_shared <config::Dict> ();
        cache-> insert ("size", (int64_t) this-> _entities [entityName].size.kilobytes ());
        cache-> insert ("ttl", (int64_t) this-> _entities [entityName].ttl);
        cache-> insert ("unit", "KB");

        auto result = config::Dict ()
            .insert ("sys", sys)
            .insert ("supervisor", super)
            .insert ("cache", cache)
            .insert ("service", service);

        return toml::dump (result);
    }

    std::string Cache::cachePath () {
        auto host = this-> _context-> getCluster ()-> get (this-> _cacheHost);
        auto path = utils::join_path (host-> getHomeDir (), "/run/caches/" + this-> _name);
        if (!this-> _hasPath) {
            host-> run ("mkdir -p " + path)-> wait ();
            host-> run ("mkdir -p " + utils::join_path (path, "res"))-> wait ();
            host-> run ("cp -r " + utils::join_path (host-> getHomeDir (), "execs/cache/libs") + " " + path)-> wait ();
            host-> run ("cp " + utils::join_path (host-> getHomeDir (), "execs/cache/supervisor") + " " + path)-> wait ();
            host-> run ("cp " + utils::join_path (host-> getHomeDir (), "execs/cache/cache") + " " + path)-> wait ();
            this-> _hasPath = true;
        }

        return path;
    }

}
