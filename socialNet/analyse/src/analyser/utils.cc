#include <vector>
#include <cstdint>
#include <iostream>

namespace analyser {

    uint64_t min (const std::vector <uint64_t> & points) {
        uint64_t min = -1;
        for (auto & x : points) {
            if (x < min) { min = x; }
        }

        return min;
    }

    uint64_t max (const std::vector <uint64_t> & points) {
        uint64_t m = 0;
        for (auto & x : points) {
            if (x > m) { m = x; }
        }

        return m;
    }

    double sum (const std::vector <uint64_t> & points) {
        double max = 0;
        for (auto & x : points) {
            max += x;
        }

        return max;
    }

    double percentile (double p, const std::vector <uint64_t> & points) {
        uint32_t index = p * points.size ();
        if (index >= points.size () - 1) {
            return points [points.size () - 1];
        }

        double y0 = points [index];
        double y1 = points [index + 1];
        double x0 = index;
        double x1 = index + 1;
        double x = p * (double) points.size ();

        double y = y0 * (x1 - x) + y1 * (x - x0); // (x1 - x0 == 1)

        return y;
    }

    double mean (const std::vector <uint64_t> & points) {
        double sum = 0;
        for (double x : points) {
            sum += x;
        }

        return sum / points.size ();
    }

    double variance (double mean, const std::vector <uint64_t> & points) {
        double sum = 0;
        for (double x : points) {
            double dist = x - mean;
            sum += (dist * dist);
        }

        return sum / points.size ();
    }



    double min (const std::vector <double> & points) {
        double min = ((uint64_t) -1);
        for (auto & x : points) {
            if (x < min) { min = x; }
        }

        return min;
    }

    double max (const std::vector <double> & points) {
        double max = 0;
        for (auto & x : points) {
            if (x > max) { max = x; }
        }

        return max;
    }

    double sum (const std::vector <double> & points) {
        double max = 0;
        for (auto & x : points) {
            max += x;
        }

        return max;
    }


    double percentile (double p, const std::vector <double> & points) {
        uint32_t index = p * points.size ();
        if (index >= points.size () - 1) {
            return points [points.size () - 1];
        }

        double y0 = points [index];
        double y1 = points [index + 1];
        double x0 = index;
        double x1 = index + 1;
        double x = p * (double) points.size ();

        double y = y0 * (x1 - x) + y1 * (x - x0); // (x1 - x0 == 1)

        return y;
    }

    double mean (const std::vector <double> & points) {
        double sum = 0;
        for (double x : points) {
            sum += x;
        }

        return sum / points.size ();
    }

    double variance (double mean, const std::vector <double> & points) {
        double sum = 0;
        for (double x : points) {
            double dist = x - mean;
            sum += (dist * dist);
        }

        return sum / points.size ();
    }


    std::vector <double> smooth (const std::vector <double> & values, uint32_t strength) {
        std::vector <double> results;
        for (uint32_t i = 0 ; i < values.size () ; i += strength) {
            double result = values [i];
            uint32_t nb = 1;
            for (uint32_t j = 0 ; j < strength ; j++) {
                if ((int32_t) i - (int32_t) j > 0) {
                    result += values [i - j];
                    nb += 1;
                }
                if (i + j < values.size ()) {
                    result += values [i + j];
                    nb += 1;
                }
            }

            results.push_back (result / nb);
        }

        return results;
    }

}
