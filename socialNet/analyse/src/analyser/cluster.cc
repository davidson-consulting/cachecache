#include "cluster.hh"


using namespace rd_utils;
using namespace rd_utils::utils;

namespace analyser {

    Cluster::Cluster () {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          CONFIGURE          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cluster::configure (const std::string & traceDir, const config::ConfigNode & cfg) {
        this-> _minTimestamp = -1;

        match (cfg) {
            of (config::Dict, dc) {
                for (auto mch : dc-> getKeys ()) {
                    this-> _machines [mch].configure (traceDir, (*dc) [mch]);
                    if (this-> _machines [mch].getMinTimestamp () < this-> _minTimestamp) {
                        this-> _minTimestamp = this-> _machines [mch].getMinTimestamp ();
                    }
                }
            } elfo {
                throw std::runtime_error ("Malformed run configuration file");
            }
        }
    }

    uint64_t Cluster::getMinTimestamp () const {
        return this-> _minTimestamp;
    }

    void Cluster::setMinTimestamp (uint64_t timestamp) {
        if (this-> _minTimestamp > timestamp) {
            this-> _minTimestamp = timestamp;
            for (auto & it : this-> _machines) {
                it.second.setMinTimestamp (this-> _minTimestamp);
            }
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          EXECUTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Cluster::execute (uint64_t timestamp, std::shared_ptr <tex::Beamer> doc) {
        this-> setMinTimestamp (timestamp);

        for (auto & it : this-> _machines) {
            it.second.execute (doc);
        }

    }


}
