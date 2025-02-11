#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <fstream>
#include <memory>


#include "../tex/_.hh"

namespace analyser {

    struct Interval {
        uint64_t begin;
        uint64_t end;
        uint64_t nb;
    };

    struct Request {
        uint64_t begin;
        uint64_t end;
        bool ok;
    };

    struct ResponseDistribution {
        uint64_t nbIntervals [100];
        uint64_t max;
        uint64_t min;
    };

    struct Percentile {
        uint64_t min;
        double p25;
        double p50;
        double p75;
        double p80;
        double p85;
        double p90;
        double p95;
        double p99;
        uint64_t max;
    };

    class Gatling {
    private:

        // The loaded requests in the log file
        std::map <std::string, std::vector <Request> > _requests;

        // The alls requests in the log file
        std::vector <Request> _allRequests;

        // The active users
        std::vector <Interval> _users;

        // The minimal timestamp of the gatling log file
        uint64_t _minTimestamp;

    public:

        Gatling ();

        /**
         * Configure the gatling traces
         * @params:
         *    - logFile: the log file
         */
        void configure (const std::string & logFile);

        /**
         * Export the traces to a beamer doc
         */
        void execute (uint32_t minTimestamp, std::shared_ptr <tex::Beamer> doc);

    private:

        /**
         * Load the trace file
         */
        void loadFile (std::ifstream & file);

        /**
         * Inject a user interval in the list of active users
         */
        void injectUser (Interval user);

        /**
         * Compute the number of responses (OK, KO)
         * @params:
         *    - intervals: the list of intervals
         */
        void computeMatrices (uint32_t minTimestamp, const std::vector <Request> & intervals, std::vector <uint64_t> & reqs, std::vector <uint64_t> & OK, std::vector <uint64_t> & KO, std::vector <Percentile> & percs);

        /**
         * Compute the distribution of intervals
         */
        ResponseDistribution computeDistribution (const std::vector <Request> & intervals);

        /**
         * Compute the percentiles of a list of points
         */
        Percentile computePercentiles (std::vector <uint64_t> & points);

        double percentile (double x, std::vector <uint64_t> & points);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          FIGURES          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Create the active user figure
         */
        void createUserFigure (uint32_t minTimestamp, std::shared_ptr <tex::Beamer> doc);

        /**
         * Create the distribution figure for a list of intervals
         */
        void createDistributionFigure (std::shared_ptr <tex::Beamer> doc, const std::string & name, const std::vector <Request> & intervals);

        /**
         * Create the figure for number of response received at each instant
         */
        void createNumberOfResponseFigure (std::shared_ptr <tex::Beamer> doc, const std::string & name, const std::vector <uint64_t> & reqs, const std::vector <uint64_t> & OK, const std::vector <uint64_t> & KO);

        /**
         * Create the figure for percentile of each responses over time
         */
        void createResponsePercentileFigure (std::shared_ptr <tex::Beamer> doc, const std::string & name, std::vector <Percentile> percs);

    };

}
