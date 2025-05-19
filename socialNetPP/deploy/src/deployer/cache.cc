#include "cache.hh"
#include "deployment.hh"
#include "cluster.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace deployer {

    rd_utils::utils::MemorySize Cache::__CACHE_FILE_SIZE__ = MemorySize::MB (128);

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
            this-> _version = cfg ["version"].getStr ();
            if (this-> _version != "disk" && this-> _version != "cachelib") {
                throw std::runtime_error ("Cache versions are 'disk' and 'cachelib'");
            }

            auto unit = cfg ["unit"].getStr ();
            this-> _size = MemorySize::unit (cfg ["size"].getI (), unit);
            if (cfg.contains ("cgroup_cache_file")) {
                __CACHE_FILE_SIZE__ = MemorySize::unit (cfg ["cgroup_cache_file"].getI (), unit);
            }

            this-> _context-> getCluster ()-> get (this-> _cacheHost)-> addFlag ("cache-" + this-> _version);
            LOG_INFO ("Register cache supervisor ", this-> _name, " of size ", this-> _size, ", on host ", this-> _cacheHost);

            match (cfg ["entities"]) {
                of (config::Dict, en) {
                    for (auto & it : en-> getKeys ()) {
                        auto entitySize = MemorySize::unit ((*en)[it]["size"].getI (), unit);

                        uint32_t ttl = (*en)[it].getOr ("ttl", 1000);
                        MemorySize diskCap = MemorySize::GB (10);
                        if ((*en)[it].contains ("disk-cap")) {
                            diskCap = MemorySize::unit ((*en)[it]["disk-cap"].getI (), unit);
                        }

                        LOG_INFO ("Register cache entity for : ", this-> _name, " ", it, " of size ", entitySize, ", with TTL = ", ttl, ", and disk capping = ", diskCap);
                        this-> _entities.emplace (it, EntityInfo {.size = entitySize, .ttl = ttl, .diskCap = diskCap});
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

    void Cache::downloadResults (const std::string & resultDir) {
        auto host = this-> _context-> getCluster ()-> get (this-> _cacheHost);
        auto dir = utils::join_path (resultDir, this-> _cacheHost);
        utils::create_directory (dir, true);

        try {
            auto superTrace = utils::join_path (utils::join_path (host-> getHomeDir (), "traces"), this-> _name + "_traces.json");
            auto outSuperPath = utils::join_path (dir, this-> _name + "_traces.json");
            host-> get (superTrace, outSuperPath);
        } catch (...) {}

        for (auto & it : this-> _entities) {
            try {
                auto entityPath = utils::join_path (utils::join_path (host-> getHomeDir (), "traces"), this-> _name + "." + it.first + "_traces.0.json");
                auto out = utils::join_path (dir, this-> _name + "." + it.first + "_traces.json");
                host-> get (entityPath, out);
            } catch (...) {}
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
        if (this-> _version != "cachelib") {
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

            host-> run ("cgclassify -g cpu,memory:cache/" + this-> _name + " " + pidStr)-> wait ();

            auto portStr = host-> getToStr (utils::join_path (path, "super_port"), 10, 0.1);
            this-> _superPort = std::strtoull (portStr.c_str (), NULL, 0);

            LOG_INFO ("Cache supervisor is running on ", this-> _cacheHost, " on port = ", this-> _superPort, ", and pid = ", pid);
            this-> _killing.push_back ("kill -2 " + std::to_string (pid));

            // Add 32 MB of memory room to prevent OOM killer in extreme cases
            auto cgroupCap = MemorySize::roundUp (this-> _size + MemorySize::MB (32), MemorySize::MB (4));
            if (this-> _size.bytes () < Cache::__CACHE_FILE_SIZE__.bytes ()) {
                cgroupCap = Cache::__CACHE_FILE_SIZE__;
            }


            LOG_INFO ("Capping cgroup cache/", this-> _name, " memory size = ", cgroupCap);
            host-> run ("cgset -r memory.high=" + std::to_string (cgroupCap.bytes ()) + " cache/" + this-> _name);

            this-> _killing.push_back ("cgset -r memory.high=max cache/" + this-> _name);
        }
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

        host-> run ("cgclassify -g cpu,memory:cache/" + this-> _name + " " + pidStr)-> wait ();

        auto portStr = host-> getToStr (utils::join_path (path, "entity_port.0." + entityName), 20, 0.05);
        auto port = std::strtoull (portStr.c_str (), NULL, 0);

        this->_entitiesPort.emplace (entityName, port);
        LOG_INFO ("Cache entity is running on ", this-> _cacheHost, " on port = ", port, ", and pid = ", pid);
        this-> _killing.push_back ("kill -2 " + std::to_string (pid));
    }

    std::string Cache::createCacheEntityConfig (const std::string & entityName, std::shared_ptr <Machine> host) {
        auto tracePath = utils::join_path (utils::join_path (host-> getHomeDir (), "traces"), this-> _name + "." + entityName + "_traces");
        auto sys = std::make_shared <config::Dict> ();
        sys-> insert ("name", entityName);
        sys-> insert ("addr", "0.0.0.0");
        sys-> insert ("port", (int64_t) 0);
        sys-> insert ("instances", (int64_t) 1);
        sys-> insert ("export-traces", tracePath);

        auto service = std::make_shared <config::Dict> ();
        service-> insert ("addr", "0.0.0.0");
        service-> insert ("port", (int64_t) 0);
        service-> insert ("nb-threads", (int64_t) 1);

        auto cache = std::make_shared <config::Dict> ();
        cache-> insert ("size", (int64_t) this-> _entities [entityName].size.kilobytes ());
        cache-> insert ("unit", "KB");
        cache-> insert ("ttl", (int64_t) this-> _entities [entityName].ttl);

        if (this-> _version != "cachelib") {
            cache-> insert ("disk-cap", (int64_t) this-> _entities [entityName].diskCap.kilobytes ());
        }

        auto result = config::Dict ()
            .insert ("sys", sys)
            .insert ("cache", cache)
            .insert ("service", service);

        if (this-> _version != "cachelib") {
            auto super = std::make_shared <config::Dict> ();
            super-> insert ("addr", "127.0.0.1");
            super-> insert ("port", (int64_t) this-> _superPort);

            result.insert ("supervisor", super);
        }

        return toml::dump (result);
    }

    std::string Cache::cachePath () {
        auto host = this-> _context-> getCluster ()-> get (this-> _cacheHost);
        auto path = utils::join_path (host-> getHomeDir (), "/run/caches/" + this-> _name);
        if (!this-> _hasPath) {
            auto cpath = "execs/";
            if (this-> _version == "cachelib") { cpath = "execs/cachelib/"; }
            else { cpath = "execs/cache-disk/"; }

            host-> run ("mkdir -p " + path)-> wait ();
            host-> run ("mkdir -p " + utils::join_path (path, "res"))-> wait ();
            host-> run ("cp -r " + utils::join_path (host-> getHomeDir (), utils::join_path (cpath, "libs")) + " " + path)-> wait ();
            host-> run ("cp " + utils::join_path (host-> getHomeDir (), utils::join_path (cpath, "supervisor")) + " " + path)-> wait ();
            host-> run ("cp " + utils::join_path (host-> getHomeDir (), utils::join_path (cpath, "cache")) + " " + path)-> wait ();

            host-> run ("cgcreate -g cpu,memory:cache/" + this-> _name)-> wait ();
            this-> _hasPath = true;
        }

        return path;
    }

}
