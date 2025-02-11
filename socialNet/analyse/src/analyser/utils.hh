#pragma once

#include <vector>
#include <cstdint>

namespace analyser {

    uint64_t min (const std::vector <uint64_t> & points);
    uint64_t max (const std::vector <uint64_t> & points);
    double sum (const std::vector <uint64_t> & points);
    double percentile (double x, const std::vector <uint64_t> & points);
    double mean (const std::vector <uint64_t> & points);
    double variance (double mean, const std::vector <uint64_t> & points);

    double min (const std::vector <double> & points);
    double max (const std::vector <double> & points);
    double sum (const std::vector <double> & points);
    double percentile (double x, const std::vector <double> & points);
    double mean (const std::vector <double> & points);
    double variance (double mean, const std::vector <double> & points);

}
