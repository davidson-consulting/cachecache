#pragma once

#include <chrono>
#include <string>

namespace cachecache {
    class Clock {
        public:
            Clock();
            Clock(Clock &) = delete;
            void operator=(Clock &) = delete;

            Clock(Clock &&);
            void operator=(Clock &&);

            unsigned int time();
            unsigned int delta(unsigned int);
    
            void update();

            std::string to_string(unsigned int);
            unsigned int from_string(std::string);

        private:
            unsigned int _time = 0;
    };
}
