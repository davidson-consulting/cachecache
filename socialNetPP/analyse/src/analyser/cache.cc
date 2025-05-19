#include "cache.hh"

#include <nlohmann/json.hpp>
#include "utils.hh"
using json = nlohmann::json;


using namespace rd_utils;
using namespace rd_utils::utils;

namespace analyser {

    Cache::Cache () {}

    void Cache::configure (uint64_t minTimestamp, const std::string & traceDir, const std::string & hostName, const std::string & name, const std::set<std::string> & entities) {
        this-> _hostName = hostName;
        this-> _name = name;
        this-> _minTimestamp = minTimestamp;

        try {
            this-> loadMarketTrace (utils::join_path (utils::join_path (traceDir, this-> _hostName), this-> _name + "_traces.json"));
        } catch (...) {
            LOG_INFO ("No market trace");
        }

        this-> _entityInMarket = entities;
        for (auto & it : entities) {
            this-> loadEntityTrace (it, utils::join_path (utils::join_path (traceDir, this-> _hostName), this-> _name + "." + it + "_traces.json"));
        }
    }

    void Cache::loadMarketTrace (const std::string & traceFile) {
        std::ifstream f(traceFile);
        json data = json::parse (f);

        for (auto & t : data) {
            uint64_t timestamp = t ["timestamp"];
            std::string unit = t ["unit"];
            if (this-> _minTimestamp > timestamp) this-> _minTimestamp = timestamp;
            this-> _trace.push_back (MarketTrace {.entitySize = {}, .poolSize = MemorySize::unit (t ["pool_size"], unit)});

            for (auto & [j, v] : t.items ()) {
                if (j != "unit" && j != "pool_size" && j != "timestamp") {
                    this-> _entityInMarket.emplace (j);
                    this-> _trace.back ().entitySize.emplace (j, MemorySize::unit (v, unit));
                }
            }
        }
    }

    void Cache::loadEntityTrace (const std::string & name, const std::string & traceFile) {
        std::ifstream f(traceFile);
        json data = json::parse (f);

        if (data.size () > 0) {
            for (uint64_t i = 0 ; i < data [0]["timestamp"].get<uint64_t> () - this-> _minTimestamp ; i++) {
                this-> _entities [name].push_back (EntityTrace {.hit = 0, .miss = 0, .set = 0, .usage = MemorySize::B (0), .size = MemorySize::B (0)});
            }
        }

        for (auto & t : data) {
            std::string unit = t ["unit"];

            uint32_t hit = t ["hit"];
            uint32_t miss = t ["miss"];
            uint32_t set = t ["set"];
            uint64_t size = t ["size"];
            uint64_t usage = t ["usage"];
            uint32_t disk_hit = 0;
            uint64_t disk_usage = 0;
            bool has_disk = false;

            try {
                disk_hit = t ["disk-hit"];
                has_disk = true;
            } catch (...) {}

            try {
                disk_usage = t ["disk-usage"];
                has_disk = true;
            } catch (...) {}

            this-> _entities [name].push_back (EntityTrace {.hit = hit, .miss = miss, .set = set, .disk_hit = disk_hit, .usage = MemorySize::unit (usage, unit), .size = MemorySize::unit (size, unit), .disk_usage = MemorySize::unit (disk_usage, unit)});
            if (has_disk) {
                this-> _entityHasDisk.emplace (name);
            }
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          EXPORT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cache::execute (std::shared_ptr <tex::Beamer> doc) {
        this-> createMarketFigure (doc);
        for (auto & it : this-> _entityInMarket) {
            this-> createEntityFigures (it, doc);
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          EXPORT MARKET          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cache::createMarketFigure (std::shared_ptr <tex::Beamer> doc) {
        std::map <std::string, std::shared_ptr <tex::Plot> > entities;
        auto pool = std::make_shared <tex::Plot>  ();
        pool-> legend ("pool");

        for (auto & usage : this-> _trace) {
            pool-> append (usage.poolSize.megabytes ());
        }

        for (auto & name : this-> _entityInMarket) {
            entities.emplace (name, std::make_shared <tex::Plot> ());
            entities [name]-> legend (name);
            entities [name]-> smooth (5);

            for (auto & usage : this-> _trace) {
                if (usage.entitySize.find (name) != usage.entitySize.end ()) {
                    entities [name]-> append (usage.entitySize [name].megabytes ());
                } else {
                    entities [name]-> append (0);
                }
            }
        }

        auto figure = tex::AxisFigure ("market_" + this-> _name)
            .caption ("Market repartition of " + this-> _name)
            .ylabel ("Memory size in MB")
            .xlabel ("seconds")
            ;

        figure.addPlot (pool);
        for (auto & it : entities) {
            figure.addPlot (it.second);
        }

        doc-> addFigure ("cache", std::make_shared <tex::AxisFigure> (figure));
    }

    void Cache::createEntityFigures (const std::string & name, std::shared_ptr <tex::Beamer> doc) {
        std::vector <double> globalRatio;

        auto usage = std::make_shared <tex::Plot> ();
        usage-> legend ("usage");

        auto disk_usage = std::make_shared <tex::Plot> ();
        disk_usage-> legend ("disk-usage").color ("red!30");

        auto size = std::make_shared <tex::Plot> ();
        size-> legend ("size");

        auto hit = std::make_shared <tex::Plot> ();
        hit-> legend ("hit").color ("green!50");

        auto disk_hit = std::make_shared <tex::Plot> ();
        disk_hit-> legend ("disk-hit").color ("orange!80");

        auto miss = std::make_shared <tex::Plot> ();
        miss-> legend ("miss").color ("red!50");

        auto set = std::make_shared <tex::Plot> ();
        set-> legend ("set");

        auto ratio = std::make_shared <tex::Plot> ();
        ratio-> legend ("ratio");

        uint64_t j = 0;
        for (auto & it : this-> _entities [name]) {
            usage-> append (it.usage.megabytes ());
            size-> append (it.size.megabytes ());
            hit-> append (it.hit);
            miss-> append (it.miss);
            set-> append (it.set);
            j += 1;

            if ((it.hit + it.miss) > 0) {
                globalRatio.push_back ((((float) it.hit / (float) (it.hit + it.miss)) * 100.0f));
            } else {
                globalRatio.push_back (0);
            }

            disk_hit-> append (it.disk_hit);
            disk_usage-> append (it.disk_usage.megabytes ());
        }

        globalRatio = smooth (globalRatio, 5);
        ratio-> factor (5);
        for (uint64_t i = 0 ; i < globalRatio.size () ; i++) {
            ratio-> append (globalRatio [i]);
        }

        auto hitFigure = tex::AxisFigure ("hit_" + this-> _name + "." + name)
            .caption ("Hit/Miss/Set of entity " + this-> _name + "." + name)
            .ylabel ("Number")
            .xlabel ("seconds")
            ;

        hitFigure.addPlot (hit).addPlot (miss).addPlot (set);

        if (this-> _entityHasDisk.find (name) != this-> _entityHasDisk.end ()) {
            hitFigure.addPlot (disk_hit);
        }

        auto sizeFigure = tex::AxisFigure ("size_" + this-> _name + "." + name)
            .caption ("Size and usage of entity " + this-> _name + "." + name)
            .ylabel ("Memory size in MB")
            .xlabel ("seconds")
            ;

        if (this-> _entityHasDisk.find (name) != this-> _entityHasDisk.end ()) {
            sizeFigure.addPlot (disk_usage);
        }

        sizeFigure.addPlot (usage).addPlot (size);

        auto ratioFigure = tex::AxisFigure ("ratio_" + this-> _name + "." + name)
            .caption ("Hit Ratio " + this-> _name + "." + name)
            .ylabel ("Hit ratio in percentage")
            .xlabel ("seconds")
            ;

        ratioFigure.addPlot (ratio);

        doc-> addFigure ("cache", std::make_shared <tex::AxisFigure> (hitFigure));
        doc-> addFigure ("cache", std::make_shared <tex::AxisFigure> (ratioFigure));
        doc-> addFigure ("cache", std::make_shared <tex::AxisFigure> (sizeFigure));
    }

}
