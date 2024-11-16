#include "cluster.hh"


namespace deployer {

    Cluster::Cluster () {}

    void Cluster::configure (const rd_utils::utils::config::ConfigNode & cfg) {
        match (cfg ["machines"]) {
            of (rd_utils::utils::config::Dict, machines) {
                try {
                    for (auto & mc : machines-> getKeys ()) {
                        auto hostname = (*machines) [mc]["hostname"].getStr ();
                        auto user = (*machines) [mc]["user"].getStr ();

                        this-> _machines.emplace (mc, std::make_shared <Machine> (hostname, user));
                    }
                } catch (const std::runtime_error & err) {
                    LOG_ERROR ("Malformed configuration file : ", err.what ());
                    throw std::runtime_error ("Malformed configuration");
                }
            } elfo {
                LOG_ERROR ("Malformed configuration file");
                throw std::runtime_error ("Malformed configuration");
            }
        }
    }

    Machine& Cluster::operator[] (const std::string & mc) {
        auto it = this-> _machines.find (mc);
        if (it == this-> _machines.end ()) {
            throw std::runtime_error ("No machine named : " + mc);
        }

        return *it-> second;
    }

    std::shared_ptr <Machine> Cluster::get (const std::string & mc) {
        auto it = this-> _machines.find (mc);
        if (it == this-> _machines.end ()) {
            throw std::runtime_error ("No machine named : " + mc);
        }

        return it-> second;
    }

    std::vector <std::string> Cluster::machines () const {
        std::vector <std::string> names;
        for (auto & it : this-> _machines) {
            names.push_back (it.first);
        }

        return names;
    }

}
