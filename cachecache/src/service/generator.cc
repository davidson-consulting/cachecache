#include <service/generator.hh>
#include <optional>
#include <rd_utils/concurrency/thread.hh>
#include <rd_utils/foreign/CLI11.hh>
#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "folly/logging/xlog.h"
#include "folly/logging/LogLevel.h"

using namespace rd_utils::concurrency;

using namespace cachecache;

//rd_utils::concurrency::signal<> exitSignal;

Generator::Generator() {
}

Generator::~Generator(){
}

Generator::Generator(Generator&& other):
    _traces(std::move(other._traces))
    , _nb_seconds(other._nb_seconds)
    , _target(std::move(other._target))
    , _clock(std::move(other._clock))
    , _threads(std::move(other._threads))
    , _finished(other._finished)
    , _stop(other._stop)
    , _ignored_lines(other._ignored_lines)
    , _time(other._time)
    , _target_time(other._target_time) {

    other._nb_seconds = 0;
    other._stop = false;
    other._ignored_lines = 0;
    other._time = 0;
    other._target_time = 0;
}

void Generator::operator=(Generator&& other) {
    this->_traces = std::move(other._traces);
    this->_nb_seconds = other._nb_seconds;
    other._nb_seconds = 0;
    this->_target = std::move(other._target);
    this->_clock = std::move(other._clock);
    this->_threads = std::move(other._threads);
    this->_finished = other._finished;
    this->_stop = other._stop;
    other._stop = false;
    this->_ignored_lines = other._ignored_lines;
    other._ignored_lines = 0;
    this->_time = 0;
    other._time = 0;

    this->_target_time = other._target_time;
    other._target_time = 1;
}

void Generator::configure(const std::string & traces, int nb_seconds, int frequency, Cachecache* target, Clock* clock, std::shared_ptr<bool> finished) {
    this->_traces = traces;
    this->_nb_seconds = nb_seconds;
    this->_target = target;
    this->_clock = clock;
    this->_finished = finished;

    this->_target_time = 1.f / (float)frequency;
}

void Generator::dispose() {
    if(!this->_stop) {
        LOG_DEBUG("Dispose")
        this->_stop = true;
    } else {
        LOG_DEBUG("Force exit");
        exit(-1);
    }
}

void Generator::run(Thread t) {
    this->run();
}

void Generator::run() {
    std::ifstream f(this->_traces);

    if(f.is_open()) {
        std::string l;
        int i = 0;

        this->_timer.reset();
        for(l; getline(f, l); ) {
            if(this->_stop) break;
            try {
                this->process(l);
            } catch (...) {
                XLOG(ERR, "ERROR WHILE PROCESSING LINE ", i);
            }
            //usleep(100);
            i++;
        }
        f.close();
        LOG_INFO("Number of ignored lines ", this->_ignored_lines);
        LOG_INFO("Current time ", this->_time);
    } else {
        LOG_ERROR("Could not open traces files at ", this->_traces);
        exit(-1);
    }

    this->_target->push_metrics();

    for(auto & t: this->_threads) {
        join(t);
    }

    XLOG(INFO, "FINISHED");
    *this->_finished = true;
}

void Generator::process(std::string& l) {
    line current = this->parseLine(l);

    if(std::atoi(current.timestamp.c_str()) != this->_time) {
        this->_clock->update();
        this->_target->push_metrics();
        this->_time++;

        if (this->_timer.time_since_start() > this->_target_time) {
            XLOG(INFO, "Processed second in ", this->_timer.time_since_start(), "s instead of ", this->_target_time, "s");
        } else {
            auto duration = this->_target_time - this->_timer.time_since_start();
            //XLOG(INFO, "Sleep for ", duration, "s");
            this->_timer.sleep(duration);
        }
        this->_timer.reset();

        if (this->_nb_seconds > 0 && std::atoi(current.timestamp.c_str()) >= this->_nb_seconds) {
            this->_stop = true;
        }
    }

    std::string value('a', current.valuesize);
    //XLOG(ERR, "Could not execute line [", l, "]");
    switch (current.operation) {
        case OPERATION::GET:
        case OPERATION::GETS:
            this->_target->get(current.key);
            break;
        case OPERATION::SET:
        case OPERATION::ADD:
            this->_target->put(current.key, value);
            break;

        default:
            XLOG(ERR, "Unsupported operation");
            this->_target->get(current.key);
    }
}


// UTILS

line Generator::parseLine(const std::string & l) const {
    line res;

    std::vector<std::string> tokens = rd_utils::utils::tokenize(l, {","}, {","});

    res.timestamp = tokens[0];
    res.key = std::to_string(std::hash<std::string>{}(tokens[1]));
    res.keysize = res.key.size(); //std::stoi(tokens[2]);
    res.valuesize = std::stoi(tokens[3]) * 2;
    res.clientid = std::stoi(tokens[4]);
    res.operation = STR_TO_OPERATION.at(tokens[5]);
    res.TTL = std::stoi(tokens[6]);

    return res;
} 
