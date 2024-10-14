#include "metrics.hh"
#include <sstream>

using namespace cachecache;

Metrics::Metrics(): _output_directory("/tmp") {}
Metrics::~Metrics() {
    for(auto &[name, ofs]: this->_ofs) {
        ofs.close();
    }
}

Metrics::Metrics(Metrics && other):
    _output_directory(std::move(other._output_directory))
    , _metrics(std::move(other._metrics))
    , _ofs(std::move(other._ofs)) {}

void Metrics::operator=(Metrics && other) {
    this->_output_directory = std::move(other._output_directory);
    this->_metrics = std::move(other._metrics);
    this->_ofs = std::move(other._ofs);
}

void Metrics::configure(const std::string & output_directory) {
    this->_output_directory = output_directory;
}

void Metrics::register_new(const std::string& name, const Labels& labels) { 
    if (this->_ofs.find(name) != this->_ofs.end()) {
        XLOG(ERR, "Register existing metric ", name);
        // throw exception
        return;
    }

   if (this->_metrics.find(name) != this->_metrics.end()) {
        this->_metrics[name].clear();
    } else {
        this->_metrics[name].reserve(labels.size());
    }
    for (const auto & [label, value]: labels) {
        this->_metrics[name].push_back(label);
    }

    std::stringstream ss;
    ss << this->_output_directory << "/" << name << ".csv";
    this->_ofs[name].open(ss.str());

    this->_ofs[name] << "time;" << name;

    if (this->_metrics[name].size() == 0) {
        this->_ofs[name] << "\n";
        return;
    }

    for (const auto & label : this->_metrics[name]) {
        this->_ofs[name] << ";" << label;
    }
    this->_ofs[name] << "\n";
    this->_ofs[name].flush();
}

void Metrics::push(const std::string& metric, const Labels& labels, const std::string& value) {
    if (this->_metrics.find(metric) == this->_metrics.end()) {
        std::scoped_lock lock(this->_mutex);
        this->register_new(metric, labels); 
    }

    std::stringstream ss;
    ss << std::to_string(this->_timer.time_since_start()) << ";" << value;
    if (this->_metrics[metric].size() == 0) {
        ss << "\n";
        return;
    }

    for (const auto & label : this->_metrics[metric]) {
        if(labels.count(label)) {
            ss << ";" << labels.at(label);
        }
    }

    ss << "\n";
    {
        //XLOG(INFO, "### ", labels.at("client") ," ASKING FOR LOCK.... ");
        std::scoped_lock lock(this->_mutex);
        //XLOG(INFO, "### ", labels.at("client") ,"GOT LOCK.... ");
        this->_ofs[metric] << ss.str();
        this->_ofs[metric].flush();
    }
}
