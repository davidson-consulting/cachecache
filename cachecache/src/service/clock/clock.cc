#include <string>

#include "clock.hh"

using namespace cachecache;
using namespace std::chrono;

Clock::Clock(): _time(0) {
}

Clock::Clock(Clock && other): _time(other._time) {
    other._time = 0;
}

void Clock::operator=(Clock && other) {
    this->_time = other._time;
    other._time = 0;
}

unsigned int Clock::time() {
    return this->_time;
}

unsigned int Clock::delta(unsigned int i) {
    return this->_time - i;
}

void Clock::update() {
    this->_time += 1;
}

std::string Clock::to_string(unsigned int t) {
    return std::to_string(t);
}

unsigned int Clock::from_string(std::string s) {
    return std::atoi(s.c_str());
}
