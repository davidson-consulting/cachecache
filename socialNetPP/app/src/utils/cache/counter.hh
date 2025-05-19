#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <rd_utils/_.hh>

namespace socialNet::utils {

    class Counter {

        std::vector <double> _time;
        double _timeSum;

        rd_utils::concurrency::mutex _m;

        uint32_t _nb;
        uint32_t _trigger;
        std::string _name;

    public:

        Counter (const std::string & name, uint32_t trigger = 100);
        void insert (double time);

        void exportTimes ();

    };

}
