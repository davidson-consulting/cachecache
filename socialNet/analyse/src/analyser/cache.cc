#include "cache.hh"

#include <nlohmann/json.hpp>
using json = nlohmann::json;


using namespace rd_utils;
using namespace rd_utils::utils;

namespace analyser {

    Cache::Cache () {}

    void Cache::configure (uint64_t minTimestamp, const std::string & traceDir, const std::string & hostName, const std::string & name) {
        this-> _hostName = hostName;
        this-> _name = name;
        this-> _minTimestamp = minTimestamp;

        this-> loadMarketTrace (utils::join_path (utils::join_path (traceDir, this-> _hostName), this-> _name + "_traces.json"));

        for (auto & it : this-> _entityInMarket) {
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

            this-> _entities [name].push_back (EntityTrace {.hit = hit, .miss = miss, .set = set, .usage = MemorySize::unit (usage, unit), .size = MemorySize::unit (size, unit)});
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
        auto usage = std::make_shared <tex::Plot> ();
        usage-> legend ("usage");

        auto size = std::make_shared <tex::Plot> ();
        size-> legend ("size");

        auto hit = std::make_shared <tex::Plot> ();
        hit-> legend ("hit").color ("green!50");

        auto miss = std::make_shared <tex::Plot> ();
        miss-> legend ("miss").color ("red!50");

        auto set = std::make_shared <tex::Plot> ();
        set-> legend ("set");


        for (auto & it : this-> _entities [name]) {
            usage-> append (it.usage.megabytes ());
            size-> append (it.size.megabytes ());
            hit-> append (it.hit);
            miss-> append (it.miss);
            set-> append (it.set);
        }

        auto hitFigure = tex::AxisFigure ("hit_" + this-> _name + "." + name)
            .caption ("Hit/Miss/Set of entity " + this-> _name + "." + name)
            .ylabel ("Number")
            .xlabel ("seconds")
            ;

        hitFigure.addPlot (hit).addPlot (miss).addPlot (set);

        auto sizeFigure = tex::AxisFigure ("size_" + this-> _name + "." + name)
            .caption ("Size and usage of entity " + this-> _name + "." + name)
            .ylabel ("Memory size in MB")
            .xlabel ("seconds")
            ;

        sizeFigure.addPlot (usage).addPlot (size);
        doc-> addFigure ("cache", std::make_shared <tex::AxisFigure> (hitFigure));
        doc-> addFigure ("cache", std::make_shared <tex::AxisFigure> (sizeFigure));
    }

}
