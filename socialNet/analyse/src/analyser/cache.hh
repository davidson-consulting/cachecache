#pragma once

#include <rd_utils/_.hh>
#include "../tex/_.hh"

namespace analyser {

    class Cache {
    private:

        struct EntityTrace {
            uint32_t hit;
            uint32_t miss;
            uint32_t set;
            rd_utils::utils::MemorySize usage;
            rd_utils::utils::MemorySize size;
        };

        struct MarketTrace {
            std::map <std::string, rd_utils::utils::MemorySize> entitySize;
            rd_utils::utils::MemorySize poolSize;
        };

    private:

        // The name of the cache
        std::string _name;

        // The name of the host on which the cache was deployed
        std::string _hostName;

    private:

        uint64_t _minTimestamp;

        // The trace of the market
        std::vector <MarketTrace> _trace;

        // The trace of the entitites
        std::map <std::string, std::vector <EntityTrace> > _entities;

        std::set <std::string> _entityInMarket;

    public:

        Cache ();

        /**
         * Configure and load the traces of the cache
         */
        void configure (uint64_t minTimestamp, const std::string & traceDir, const std::string & hostName, const std::string & name);

        /**
         * Export the traces to a beamer doc
         */
        void execute (std::shared_ptr <tex::Beamer> doc);

    private:

        /**
         * Load the traces of the market
         */
        void loadMarketTrace (const std::string & traceFile);

        /**
         * Load the traces of an entity
         */
        void loadEntityTrace (const std::string & entityName, const std::string & traceFile);

        /**
         * Create the figure from market traces
         */
        void createMarketFigure (std::shared_ptr <tex::Beamer> doc);

        /**
         * Create the figures for a given entity
         */
        void createEntityFigures (const std::string & name, std::shared_ptr <tex::Beamer> doc);

    };


}
