#include "installer.hh"
#include "cluster.hh"
#include "machine.hh"


using namespace rd_utils;
using namespace rd_utils::utils;

namespace deployer {

    Installer::Installer (Cluster & c)
        : _context (&c)
    {}

    void Installer::execute () {
        for (auto & mc : this-> _context-> machines ()) {
            this-> _threads.push_back (concurrency::spawn (this, &Installer::installOnNode, mc));
        }

        for (auto & it : this-> _threads) {
            concurrency::join (it);
        }
    }

    void Installer::installOnNode (concurrency::Thread, std::string mc) {
        auto mch = this-> _context-> get (mc);
        this-> runAptInstalls (mch);

        if (mch-> hasFlag ("social") || mch-> hasFlag ("cache")) {
            this-> runConfigureVjoule (mch);
            this-> runRdUtilsInstall (mch);
        }

        if (mch-> hasFlag ("social")) {
            this-> runHttpInstall (mch);
            this-> runSocialNetInstall (mch);
        }

        if (mch-> hasFlag ("gatling")) {
            this-> runGatlingInstall (mch);
        }

        if (mch-> hasFlag ("cache-disk")) {
            this-> runCacheInstall (mch, "disk");
        } else if (mch-> hasFlag ("cache-cachelib")) {
            this-> runCacheInstall (mch, "cachelib");
        }
    }

    void Installer::runAptInstalls (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install apt dependencies on : ", m-> getName ());
        auto script =
            "apt update\n"
            "apt install -y mysql-client libmysqlclient-dev libgc-dev emacs patchelf net-tools cgroup-tools\n"
            "apt install -y docker-compose git cmake g++ binutils automake libtool libmicrohttpd-dev libbson-dev libmongoc-dev\n"
            "apt install -y nlohmann-json3-dev libgc-dev libssh-dev libssl-dev libmysqlcppconn-dev libmysql++-dev zip\n"
            // "wget DEPENDENCY HIDDEN FOR ANONIMITY REASONS"
            "dpkg -i vjoule-tools_1.3.0.deb\n"
            "rm vjoule-tools_1.3.0.deb\n"
            "mkdir " + m-> getHomeDir () + "/traces/\n";

        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }


    void Installer::runHttpInstall (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install libhttpd on : ", m-> getName ());
        auto script = "mkdir -p " + m-> getHomeDir () + "\n"
            "cd " + m-> getHomeDir () + "\n"
            "git clone https://github.com/etr/libhttpserver.git\n"
            "cd libhttpserver\n"
            "./bootstrap\n"
            "mkdir " + m-> getHomeDir () + "/libhttpserver/build\n"
            "cd " + m-> getHomeDir () + "/libhttpserver/build\n"
            "../configure\n"
            "make\n"
            "make install\n"
            "mv /usr/local/lib/libhttp* /usr/lib/\n"
            "rm -rf " + m-> getHomeDir () + "/libhttpserver\n"
            ;

        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    void Installer::runRdUtilsInstall (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install rd_utils on : ", m-> getName ());
        auto script = "mkdir -p " + m-> getHomeDir () + "\n"
            "cd " + m-> getHomeDir () + "\n"
            // "GIT_SSH_COMMAND=\"ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no\" git clone --depth=1 DEPENDENCY HIDDEN FOR ANONIMITY REASONS\n"
            "cd rd_utils\n"
            "mkdir .build\n"
            "cd .build\n"
            "cmake ..\n"
            "make -j12\n"
            "make install\n"
            "cd " + m-> getHomeDir () + "\n"
            "rm -rf rd_utils\n"
            ;
        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    void Installer::runSocialNetInstall (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install socialNet on : ", m-> getName ());
        auto script = "mkdir -p " + m-> getHomeDir () + "\n"
            "cd " + m-> getHomeDir () + "\n"
            // "GIT_SSH_COMMAND=\"ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no\" git clone --depth=1 DEPENDENCY HIDDEN FOR ANONIMITY REASONS\n"
            "cd socialNet/\n"
            "git checkout webapp\n"
            "cd socialNet/app\n"
            "mkdir .build\n"
            "cd .build\n"
            "cmake ..\n"
            "make -j12\n"
            "mkdir -p " + m-> getHomeDir () + "/execs/socialNet\n"
            "mv front " + m-> getHomeDir () + "/execs/socialNet\n"
            "mv reg " + m-> getHomeDir () + "/execs/socialNet\n"
            "mv services " + m-> getHomeDir () + "/execs/socialNet\n"
            "cd " + m-> getHomeDir () + "\n"
            // "rm -rf socialNet\n"
            ;

        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    void Installer::runCacheInstall (std::shared_ptr <Machine> m, const std::string & version) {
        LOG_INFO ("Install cache (", version, ") on : ", m-> getName ());
        auto script = "mkdir -p " + m-> getHomeDir () + "\n"
            "cd " + m-> getHomeDir () + "\n"
            // "GIT_SSH_COMMAND=\"ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no\" git clone --depth=1 DEPENDENCY HIDDEN FOR ANONIMITY REASONS\n"
            "cd cachecache/\n"
            "git checkout " + version + "\n"
            ;

        if (version == "disk") {
            script += "mkdir .build\n"
                "cd .build\n"
                "cmake ..\n"
                "make -j12\n"
                "mkdir -p " + m-> getHomeDir () + "/execs/cache-disk\n"
                "mv " + m-> getHomeDir () + "/cachecache/.build/supervisor " + m-> getHomeDir () + "/execs/cache-disk/\n"
                "mv " + m-> getHomeDir () + "/cachecache/.build/cache " + m-> getHomeDir () + "/execs/cache-disk/\n"
                // "rm -rf " + m-> getHomeDir () + "/cachecache/"
                ;
        } else if (version == "cachelib") {
            script += "mkdir .build\n"
                "./clone_cachelib.sh\n"
                "./build.sh\n"
                "cd .build\n"
                "cmake ..\n"
                "make -j12\n"
                "mkdir -p " + m-> getHomeDir () + "/execs/cachelib\n"
                "mkdir " + m-> getHomeDir () + "/execs/cachelib/libs\n"
                "mv " + m-> getHomeDir () + "/cachecache/CacheLib/opt/cachelib/lib/lib* " + m-> getHomeDir () + "/execs/cachelib/libs\n"
                "mv " + m-> getHomeDir () + "/cachecache/.build/supervisor " + m-> getHomeDir () + "/execs/cachelib/\n"
                "mv " + m-> getHomeDir () + "/cachecache/.build/cache " + m-> getHomeDir () + "/execs/cachelib/\n"
                "cd " + m-> getHomeDir () + "/execs/cachelib\n"
                "for j in {0..3};\n"
                "do\n"
                "    for i in $(ldd supervisor | grep \"not found\" | awk '{ print $1 }');\n"
                "    do\n"
                "        patchelf --add-needed ./libs/${i} supervisor;\n"
                "    done\n"
                "done\n"
                "\n"
                "for j in {0..3};\n"
                "do\n"
                "    for i in $(ldd cache | grep \"not found\" | awk '{ print $1 }');\n"
                "    do\n"
                "        patchelf --add-needed ./libs/${i} cache;\n"
                "    done\n"
                "done\n"
                "\n"
                // "rm -rf " + m-> getHomeDir () + "/cachecache/"
                ;
        } else {
            LOG_ERROR ("Unkown cache version : ", version);
        }


        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    void Installer::runGatlingInstall (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install gatling on : ", m-> getName ());
        auto script = "echo \"deb https://repo.scala-sbt.org/scalasbt/debian all main\" | sudo tee /etc/apt/sources.list.d/sbt.list\n"
            "echo \"deb https://repo.scala-sbt.org/scalasbt/debian /\" | sudo tee /etc/apt/sources.list.d/sbt_old.list\n"
            "curl -sL \"https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823\" | sudo apt-key add\n"
            "sudo apt-get update\n"
            "sudo apt-get install -y sbt openjdk-18-jdk\n"
            // "GIT_SSH_COMMAND=\"ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no\" git clone --depth=1 DEPENDENCY HIDDEN FOR ANONIMITY REASONS\n"
            "cd cachecache/\n"
            "git checkout webapp\n"
            "mkdir " + m-> getHomeDir () + "/gatling\n"
            "mv web-apps/socialNet/workload/ " + m-> getHomeDir () + "/gatling\n"
            // "rm -rf cachecache/\n"
            ;

        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          VJOULE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Installer::runConfigureVjoule (std::shared_ptr <Machine> m) {
        auto cfg = this-> createVjouleConfig (m);
        m-> putFromStr (cfg, "/etc/vjoule/config.toml");

        auto cgroupFile =
            "cache/*\n"
            "apps/*\n"
            "system.slice/docker-*"
            ;

        m-> putFromStr (cgroupFile, "/etc/vjoule/cgroups");
    }

    std::string Installer::createVjouleConfig (std::shared_ptr <Machine> m) const {
        auto sensor = std::make_shared <config::Dict> ();
        sensor-> insert ("freq", (int64_t) 1);
        sensor-> insert ("log-lvl", "none");
        sensor-> insert ("core", "dumper");
        sensor-> insert ("log-path", "/etc/vjoule/service.log");
        sensor-> insert ("output-dir", "/etc/vjoule/results");
        sensor-> insert ("mount-tmpfs", std::make_shared <config::Bool> (true));

        auto counters = std::make_shared <config::Array> ();
        counters-> insert ("PERF_COUNT_SW_CPU_CLOCK");
        sensor-> insert ("perf-counters", counters);

        auto cpu = std::make_shared <config::Dict> ();
        cpu-> insert ("name", "rapl");

        auto ram = std::make_shared <config::Dict> ();
        ram-> insert ("name", "rapl");

        auto result = config::Dict ()
            .insert ("sensor", sensor)
            .insert ("cpu", cpu)
            .insert ("ram", ram);

        if (m-> hasPDU ()) {
            auto pdu = std::make_shared <config::Dict> ();
            pdu-> insert ("name", "yocto");
            result.insert ("pdu", pdu);
        }

        return toml::dump (result);
    }

}
