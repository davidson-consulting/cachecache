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
                        auto entitySize = MemorySize::unit ((*en)[it].getI (), unit);
                        LOG_INFO ("Register cache entity for : ", this-> _name, " ", it, " of size ", entitySize.megabytes (), "MB");
                        this-> _entities.emplace (it, entitySize);
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

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          EXECUTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cache::start () {
        this-> deploySupervisor ();
        for (auto & it : this-> _entities) {
            this-> deployCacheEntity (it.first);
        }
    }

    void Cache::join () {
        for (auto & it : this-> _running) {
            it-> wait ();
        }

        this-> _running.clear ();
        this-> kill ();
    }

    void Cache::kill () {
        LOG_INFO ("Kill cache ", this-> _name);
        for (auto & it : this-> _running) {
            it-> dispose ();
        }

        this-> _running.clear ();
        for (auto & it : this-> _killing) {
            this-> _context-> getCluster ()-> get (this-> _cacheHost)-> run (it)-> wait ();
        }
        this-> _killing.clear ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ==================================          DEPLOYEMENT          ===================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cache::deploySupervisor () {
        auto host = this-> _context-> getCluster ()-> get (this-> _cacheHost);
        auto cfg = this-> createCacheSupervisorConfig (host);

        auto script = "cd ~/cachecache/cachecache/.build\n"
            "./supervisor -c ../res/super.toml | tee super.out.txt\n";

        host-> putFromStr (cfg, utils::join_path (host-> getHomeDir (), "/cachecache/cachecache/res/super.toml"));
        LOG_INFO ("Launching cache supervisor on ", this-> _cacheHost);

        this-> _running.push_back (host-> runScript (script));

        concurrency::timer t;
        LOG_INFO ("Waiting for supervisor to be avaible");
        t.sleep (1);

        auto portStr = host-> getToStr (utils::join_path (host-> getHomeDir (), "/cachecache/cachecache/.build/super_port"));
        this-> _superPort = std::strtoull (portStr.c_str (), NULL, 0);

        LOG_INFO ("Cache supervisor is running on ", this-> _cacheHost, " on port : ", this-> _superPort);
        this-> _killing.push_back ("pkill -9 supervisor");
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

    void Cache::deployCacheEntity (const std::string & entityName) {}

}
