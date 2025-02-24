#include "gatling.hh"
#include <rd_utils/utils/log.hh>
#include <list>
#include <algorithm>
#include <cstring>
#include <rd_utils/utils/print.hh>
#include <math.h>

#include "utils.hh"

namespace analyser {

    Gatling::Gatling () {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Gatling::configure (const std::string & logFile) {
        std::ifstream f (logFile);
        if (!f.is_open ()) throw std::runtime_error ("Failed to load gatling file");
        this-> loadFile (f);
    }

    void Gatling::loadFile (std::ifstream & file) {
        std::list <uint64_t> startedUsers;

        std::string line;
        while (std::getline (file, line)) {
            std::istringstream iss (line);
            std::string info;
            if (!(iss >> info)) { break; }

            if (info == "RUN") { continue; } // header
            if (info == "USER") {
                std::string ig, type;
                uint64_t timestamp;
                iss >> ig >> type >> timestamp;
                if (type == "START") { startedUsers.push_back (timestamp); }
                else {
                    uint64_t beg = startedUsers.front ();
                    startedUsers.pop_front ();
                    this-> injectUser (Interval {.begin = beg, .end = timestamp, .nb = 1});
                }
            } else if (info == "REQUEST") {
                std::string name, type;
                uint64_t beg, end;
                iss >> name >> beg >> end >> type;
                this-> _requests [name].push_back (Request {.begin = beg, .end = end, .ok = (type == "OK")});
                this-> _allRequests.push_back (Request {.begin = beg, .end = end, .ok = (type == "OK")});
            } else {
                LOG_WARN ("Unknwon info in gatling file : ", info);
            }
        }

        this-> _minTimestamp = 0xFFFFFFFFFFFFFFFF;
        for (auto & it : this-> _requests) {
            std::sort (it.second.begin (), it.second.end (), [](const Request & a, const Request & b) {
                return a.begin < b.begin;
            });

            if (it.second.size () > 0 && it.second [0].begin < this-> _minTimestamp) {
                this-> _minTimestamp = it.second [0].begin;
            }
        }
    }

    void Gatling::injectUser (Interval user) {
        Interval b = user;
        std::vector <Interval> result;
        for (auto & a : this-> _users) {
            if (b.begin <= a.end && b.end >= a.begin) {
                auto cB = std::max (a.begin, b.begin);
                auto cE = std::min (a.end, b.end);
                auto c = Interval {.begin = cB, .end = cE, .nb = b.nb + a.nb};
                uint64_t nbPrev = 0;
                uint64_t nbNext = 0;
                if (b.begin < a.begin) { nbPrev = b.nb; nbNext = a.nb; }
                else { nbPrev = a.nb; nbNext = b.nb; }

                auto prev = Interval {.begin = std::min (a.begin, b.begin), .end = cB, .nb = nbPrev};
                auto next = Interval {.begin = cE, .end = std::max (a.end, b.end), .nb = nbNext};

                if (prev.begin != prev.end) {
                    result.push_back (prev);
                }
                if (c.begin != c.end) {
                    result.push_back (c);
                }

                if (next.begin != next.end) {
                    b = next;
                } else return;
            } else {
                result.push_back (a);
            }
        }

        if (b.begin != b.end) { result.push_back (b); }
        this-> _users = std::move (result);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          EXECUTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Gatling::execute (uint64_t minTimestamp, std::shared_ptr <tex::Beamer> doc) {
        this-> createUserFigure (minTimestamp, doc);

        if (this-> _allRequests.size () != 0) {
            this-> createDistributionFigure (doc, "ALL", this-> _allRequests);
            std::vector <uint64_t> begins;
            std::vector <uint64_t> OK, KO;
            std::vector <Percentile> percs;
            this-> computeMatrices (minTimestamp, this-> _allRequests, begins, OK, KO, percs);
            this-> createNumberOfResponseFigure (doc, "ALL", begins, OK, KO);
            this-> createResponsePercentileFigure (doc, "ALL", percs);
        }

        for (auto & it : this-> _requests) {
            this-> createDistributionFigure (doc, it.first, it.second);
            std::vector <uint64_t> begins;
            std::vector <uint64_t> OK, KO;
            std::vector <Percentile> percs;
            this-> computeMatrices (minTimestamp, it.second, begins, OK, KO, percs);
            this-> createNumberOfResponseFigure (doc, it.first, begins, OK, KO);
            this-> createResponsePercentileFigure (doc, it.first, percs);
        }
    }

    void Gatling::createUserFigure (uint64_t minTimestamp, std::shared_ptr <tex::Beamer> doc) {
        auto pl = std::make_shared <tex::IndexedPlot> ();
        pl-> legend ("active users");

        for (auto & it : this-> _users) {
            double x = it.begin - (((uint64_t) minTimestamp) * 1000);
            pl-> append (x / 1000, it.nb);
        }

        auto figure = tex::AxisFigure ("gatling_users")
            .caption ("Active users during gatling execution")
            .ylabel ("Number of users")
            .xlabel ("seconds")
            .smooth (true);

        figure.addPlot (pl);
        doc-> addFigure ("gatling users", std::make_shared <tex::AxisFigure> (figure));
    }


    void Gatling::createDistributionFigure (std::shared_ptr <tex::Beamer> doc, const std::string & name, const std::vector <Request> & reqs) {
        auto pl = std::make_shared <tex::IndexedPlot> ();
        pl-> legend ("distribution");
        pl-> style ("ybar interval");


        Percentile percs;
        auto distrib = this-> computeDistribution (reqs, percs);

        for (uint64_t i = 0 ; i < 100 ; i++) {
            double index = ((double) i * (double) (distrib.max - distrib.min)) / 100;
            pl-> append (index + distrib.min, ((double) distrib.nbIntervals [i] / (double) reqs.size ()) * 100);
        }

        auto table = tex::TableFigure ("table_gatling_distrib_" + name, {"", "ms"})
            .caption ("Response time");

        table.addRow ({"min", std::to_string ((uint64_t) percs.min)});
        table.addRow ({"max", std::to_string ((uint64_t) percs.max)});
        table.addRow ({"mean", std::to_string ((uint64_t) distrib.mean)});
        table.addRow ({"std", std::to_string ((uint64_t) ::sqrt (distrib.variance))});
        table.addRow ({});
        table.addRow ({});
        table.addRow ({"25th", std::to_string ((uint64_t) percs.p25)});
        table.addRow ({"50th", std::to_string ((uint64_t) percs.p50)});
        table.addRow ({"75th", std::to_string ((uint64_t) percs.p75)});
        table.addRow ({"80th", std::to_string ((uint64_t) percs.p80)});
        table.addRow ({"85th", std::to_string ((uint64_t) percs.p85)});
        table.addRow ({"90th", std::to_string ((uint64_t) percs.p90)});
        table.addRow ({"95th", std::to_string ((uint64_t) percs.p95)});
        table.addRow ({"99th", std::to_string ((uint64_t) percs.p99)});

        auto figure = tex::AxisFigure ("gatling_distrib_" + name)
            .caption ("Response time distribution - req = " + name)
            .ylabel ("Percentage of requests")
            .xlabel ("ms")
            .hist (true);

        figure.addPlot (pl);

        auto minipage = tex::MiniPageFigure ("minipage_gatling_distrib_" + name)
            .addFigure (std::make_shared <tex::AxisFigure> (figure), 75)
            .addFigure (std::make_shared <tex::TableFigure> (table), 18);


        doc-> addFigure ("gatling distributions", std::make_shared <tex::MiniPageFigure> (minipage));
    } 

    void Gatling::createNumberOfResponseFigure (std::shared_ptr <tex::Beamer> doc, const std::string & name, const std::vector <uint64_t> & reqs, const std::vector <uint64_t> & OK, const std::vector <uint64_t> & KO) {
        auto plReq = std::make_shared <tex::Plot> ();
        plReq-> legend ("Requests");
        for (auto & it : reqs) {
            plReq-> append (it);
        }

        auto plOk = std::make_shared <tex::Plot> ();
        plOk-> legend ("OK");

        for (auto & it : OK) {
            plOk-> append (it);
        }

        auto plKO = std::make_shared <tex::Plot> ();
        plKO-> legend ("KO").color ("red!40");

        for (auto & it : KO) {
            plKO-> append (it);
        }

        auto figure = tex::AxisFigure ("gatling_resps_" + name)
            .caption ("Number of responses per seconds - req = " + name)
            .ylabel ("Number of responses")
            .xlabel ("seconds")
            ;

        figure.addPlot (plReq);
        figure.addPlot (plOk);
        figure.addPlot (plKO);
        doc-> addFigure ("gatling distributions", std::make_shared <tex::AxisFigure> (figure));
    }

    void Gatling::createResponsePercentileFigure (std::shared_ptr <tex::Beamer> doc, const std::string & name, std::vector <Percentile> percs) {
        auto min = std::make_shared <tex::Plot> ();
        auto max = std::make_shared <tex::Plot> ();
        auto p25 = std::make_shared <tex::Plot> ();
        auto p50 = std::make_shared <tex::Plot> ();
        auto p75 = std::make_shared <tex::Plot> ();
        auto p80 = std::make_shared <tex::Plot> ();
        auto p85 = std::make_shared <tex::Plot> ();
        auto p90 = std::make_shared <tex::Plot> ();
        auto p95 = std::make_shared <tex::Plot> ();
        auto p99 = std::make_shared <tex::Plot> ();

        min-> legend ("min").name ("min").color ("blue!50");
        p25-> legend ("25th").name ("a").color ("green!20");
        p50-> legend ("50th").name ("b").color ("green!40");
        p75-> legend ("75th").name ("c").color ("green!50");
        p80-> legend ("80th").name ("d").color ("red!20");
        p85-> legend ("85th").name ("e").color ("red!50");
        p90-> legend ("90th").name ("f").color ("purple!30");
        p95-> legend ("95th").name ("g").color ("purple!60");
        p99-> legend ("99th").name ("h").color ("black!50");
        max-> legend ("max").name ("max").color ("black!90");

        for (auto & it : percs) {
            min-> append (it.min);
            max-> append (it.max);
            p25-> append (it.p25);
            p50-> append (it.p50);
            p75-> append (it.p75);
            p80-> append (it.p80);
            p85-> append (it.p85);
            p90-> append (it.p90);
            p95-> append (it.p95);
            p99-> append (it.p99);
        }

        auto figure = tex::AxisFigure ("gatling_resps_percs_" + name)
            .caption ("Response time percentiles over time - req = " + name)
            .ylabel ("Response time (ms)")
            .xlabel ("seconds")
            .nbColumnLegend (6)
            ;

        figure.addPlot (min);
        figure.addPlot (max);
        figure.addPlot (p25);
        figure.addPlot (p50);
        figure.addPlot (p75);
        figure.addPlot (p80);
        figure.addPlot (p85);
        figure.addPlot (p90);
        figure.addPlot (p95);
        figure.addPlot (p99);
        figure.addFillPlot ("min", "a", "green!10");
        figure.addFillPlot ("a", "b", "green!30");
        figure.addFillPlot ("b", "c", "green!45");
        figure.addFillPlot ("c", "d", "red!10");
        figure.addFillPlot ("d", "e", "red!30");
        figure.addFillPlot ("e", "f", "purple!30");
        figure.addFillPlot ("f", "g", "purple!55");
        figure.addFillPlot ("g", "h", "black!10");
        figure.addFillPlot ("h", "max", "black!50");

        doc-> addFigure ("gatling distributions", std::make_shared <tex::AxisFigure> (figure));
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ==================================          COMPUTATION          ===================================
     * ====================================================================================================
     * ====================================================================================================
     */

    ResponseDistribution Gatling::computeDistribution (const std::vector <Request> & interval, Percentile & percs) {
        ResponseDistribution distrib;
        ::memset (distrib.nbIntervals, 0, 100 * sizeof (uint64_t));

        uint64_t max = 0;
        uint64_t min = 0xFFFFFFFFFFFFFFFF;
        for (auto & i : interval) {
            auto len = i.end - i.begin;
            if (max < len) { max = len; }
            if (min > len) { min = len; }
        }

        std::vector <uint64_t> points;
        for (auto & i : interval) {
            auto len = i.end - i.begin;
            uint64_t perc = ((double) (len - min) / (double) (max - min)) * 100;
            distrib.nbIntervals [perc] += 1;
            points.push_back (len);
        }

        std::sort (points.begin (), points.end ());

        percs.min = min;
        percs.max = max;
        percs.p25 = analyser::percentile (0.25, points);
        percs.p50 = analyser::percentile (0.50, points);
        percs.p75 = analyser::percentile (0.75, points);
        percs.p80 = analyser::percentile (0.80, points);
        percs.p85 = analyser::percentile (0.85, points);
        percs.p90 = analyser::percentile (0.90, points);
        percs.p95 = analyser::percentile (0.95, points);
        percs.p99 = analyser::percentile (0.99, points);

        distrib.min = min;
        distrib.max = max;
        distrib.mean = analyser::mean (points);
        distrib.variance = analyser::variance (distrib.mean, points);
        return distrib;
    }

    void Gatling::computeMatrices (uint64_t minTimestamp, const std::vector <Request> & interval, std::vector <uint64_t> & reqs, std::vector <uint64_t> & OK, std::vector <uint64_t> & KO, std::vector <Percentile> & percs) {
        uint64_t max = 0;
        for (auto & i : interval) {
            auto len = i.end;
            if (max < len) { max = len; }
        }

        std::vector <std::vector <uint64_t> > computePercs;
        uint64_t len = (max - ((uint64_t) minTimestamp) * 1000) / 1000;

        reqs.resize (len + 1);
        OK.resize (len + 1);
        KO.resize (len + 1);
        percs.resize (len + 1);
        computePercs.resize (len + 1);

        for (auto & i : interval) {
            uint64_t bi = (i.begin - (((uint64_t) minTimestamp) * 1000)) / 1000;
            uint64_t ei = (i.end - (((uint64_t) minTimestamp) * 1000)) / 1000;
            uint64_t len = i.end - i.begin;

            reqs [bi] += 1;
            if (i.ok) {
                OK [ei] += 1;
            } else { KO [ei] += 1; }

            computePercs [ei].push_back (len);
        }

        for (uint64_t i = 0 ; i < len ; i++) {
            percs [i] = this-> computePercentiles (computePercs [i]);
        }
    }

    Percentile Gatling::computePercentiles (std::vector <uint64_t> & points) {
        std::sort (points.begin (), points.end ());
        Percentile result;
        if (points.size () > 0) {
            result.min = points [0];
            result.max = points [points.size () - 1];

            result.p25 = analyser::percentile (0.25, points);
            result.p50 = analyser::percentile (0.50, points);
            result.p75 = analyser::percentile (0.75, points);
            result.p80 = analyser::percentile (0.80, points);
            result.p85 = analyser::percentile (0.85, points);
            result.p90 = analyser::percentile (0.90, points);
            result.p95 = analyser::percentile (0.95, points);
            result.p99 = analyser::percentile (0.99, points);
        } else {
            ::memset (&result, 0, sizeof (Percentile));
        }
        
        return result;
    }

}
