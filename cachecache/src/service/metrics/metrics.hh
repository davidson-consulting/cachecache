#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "folly/logging/xlog.h"
#include "folly/logging/LogLevel.h"
#include <rd_utils/concurrency/timer.hh>
#include <service/clock/clock.hh>

namespace cachecache {
    // An association between a label category (e.g "Client") and its value (e.g "client_1")
    typedef std::pair<std::string, std::string> Label;
    typedef std::unordered_map<std::string, std::string> Labels;
    class Metrics {
        public:
            Metrics();
            ~Metrics();

            Metrics(Metrics &) = delete;
            void operator=(Metrics &) = delete;
            Metrics(Metrics &&);
            void operator=(Metrics &&);

            void configure(const std::string& output_directory);
            void push(const std::string& metric, const Labels& labels, const std::string& value);

        private:
            std::mutex _mutex;
            rd_utils::concurrency::timer _timer;

            std::string _output_directory;
            // a mapping between a metric name and its associated label names
            std::unordered_map<std::string, std::vector<std::string>> _metrics;
            // mapping between a metric name and its output file stream
            std::unordered_map<std::string, std::ofstream> _ofs;

            void register_new(const std::string& metric, const Labels& labels);
    };
}
