#pragma once 

#include <array>

// P^2 algo impl based on (https://web.archive.org/web/20240104104103/https://aakinshin.net/posts/p2-quantile-estimator/)
namespace cachecache {
    class Percentile {
        public:
            Percentile();
            explicit Percentile(double percentile);

            Percentile(const Percentile&) = delete;
            void operator=(const Percentile&) = delete;

            Percentile(Percentile&&);
            void operator=(Percentile&&);
            
            void setPercentile(double percentile);
            double getPercentile() const;
            
            void addValue(double);

            double getEstimation() const;

            double getIndex() const;
            double getCount() const;

            void reset();

        private:
            double _p;
            std::array<int, 5> _n; // marker positions
            std::array<double, 5> _ns; // desired marker positions
            std::array<double, 5> _q; // marker heights

            unsigned int _count;

            double parabolic(int i, double d) const;
            double linear(int i, double d) const;
    };
}
