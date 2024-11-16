#include "installer.hh"
#include "cluster.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace deployer {

    Installer::Installer (Cluster & c, const std::string & wd)
        : _wd (wd)
        , _context (&c)
    {}

    void Installer::execute (const config::ConfigNode & cfg) {
        for (auto & mc : this-> _context-> machines ()) {
            auto scripts = this-> getInstallScripts (mc, cfg);
            WITH_LOCK (this-> _m) {
                this-> _scripts.emplace (mc, scripts);
            }
            this-> _threads.push_back (concurrency::spawn (this, &Installer::runScripts, mc));
        }

        for (auto & it : this-> _threads) {
            concurrency::join (it);
        }
    }


    std::vector <std::string> Installer::getInstallScripts (const std::string & nodeName, const config::ConfigNode & cfg) {
        std::vector <std::string> result;
        try {
            match (cfg ["machines"][nodeName]["installer"]) {
                of (config::Array, arr) {
                    for (uint32_t i = 0 ; i < arr-> getLen () ; i++) {
                        auto str = join_path (this-> _wd, (*arr) [i].getStr ());
                        result.push_back (str);
                    }
                } fo;
            }
        } catch (...) {
            return {};
        }

        return result;
    }

    void Installer::runScripts (concurrency::Thread, std::string mc) {
        std::vector <std::string> scripts;
        WITH_LOCK (this-> _m) {
            auto it = this-> _scripts.find (mc);
            if (it != this-> _scripts.end ()) {
                scripts = it-> second;
            }
        }

        auto mch = this-> _context-> get (mc);
        for (auto & script : scripts) {
            LOG_INFO ("Running : ", script, " on ", mc);
            auto filename = utils::get_filename (script);
            mch-> put (script, "/tmp/");
            auto proc = mch-> run ("bash /tmp/" + filename);
            proc-> wait ();

            LOG_INFO ("On ", mc, " : ", proc-> stdout ());
        } 
    }

}
