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
            this-> runRdUtilsInstall (mch);
        }

        if (mch-> hasFlag ("social")) {
            this-> runHttpInstall (mch);
            this-> runSocialNetInstall (mch);
        }

        if (mch-> hasFlag ("gatling")) {
            this-> runGatlingInstall (mch);
        }

        if (mch-> hasFlag ("cache")) {
            this-> runCacheInstall (mch);
        }
    }

    void Installer::runAptInstalls (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install apt dependencies on : ", m-> getName ());
        auto script =
            "apt update\n"
            "apt install -y mysql-client libmysqlclient-dev libgc-dev emacs patchelf net-tools\n"
            "apt install -y docker-compose git cmake g++ binutils automake libtool libmicrohttpd-dev\n"
            "apt install -y nlohmann-json3-dev libgc-dev libssh-dev libssl-dev libmysqlcppconn-dev libmysql++-dev\n"
            "wget https://github.com/davidson-consulting/vjoule/releases/download/v1.3.0/vjoule-tools_1.3.0.deb\n"
            "dpkg -i vjoule-tools_1.3.0.deb\n"
            "rm vjoule-tools_1.3.0.deb\n"
            "mkdir ~/traces/\n";

        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    void Installer::runHttpInstall (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install libhttpd on : ", m-> getName ());
        auto script = "cd ~\n"
            "git clone https://github.com/etr/libhttpserver.git\n"
            "cd libhttpserver\n"
            "./bootstrap\n"
            "mkdir ~/libhttpserver/build\n"
            "cd ~/libhttpserver/build\n"
            "../configure\n"
            "make\n"
            "make install\n"
            "mv /usr/local/lib/libhttp* /usr/lib/\n"
            "rm -rf ~/libhttpserver\n";

        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    void Installer::runRdUtilsInstall (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install rd_utils on : ", m-> getName ());
        auto script = "cd ~\n"
            "GIT_SSH_COMMAND=\"ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no\" git clone --depth=1 git@github.com:davidson-consulting/rd_utils.git\n"
            "cd rd_utils\n"
            "mkdir .build\n"
            "cd .build\n"
            "cmake ..\n"
            "make -j12\n"
            "make install\n"
            "cd ~\n"
            "rm -rf rd_utils\n"
            ;
        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    void Installer::runSocialNetInstall (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install socialNet on : ", m-> getName ());
        auto script = "cd ~\n"
            "GIT_SSH_COMMAND=\"ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no\" git clone git@github.com:davidson-consulting/cachecache.git\n"
            "cd cachecache/\n"
            "git checkout net\n"
            "cd web-apps/socialNet/cpp\n"
            "mkdir .build\n"
            "cd .build\n"
            "cmake ..\n"
            "make -j12\n"
            "mkdir -p ~/execs/socialNet\n"
            "mv front ~/execs/socialNet\n"
            "mv reg ~/execs/socialNet\n"
            "mv services ~/execs/socialNet\n"
            "cd ~\n"
            "rm -rf cachecache\n"
            ;

        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }

    void Installer::runCacheInstall (std::shared_ptr <Machine> m) {
        LOG_INFO ("Install cache on : ", m-> getName ());
        auto script = "cd ~\n"
            "GIT_SSH_COMMAND=\"ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no\" git clone git@github.com:davidson-consulting/cachecache.git\n"
            "cd cachecache/\n"
            "git checkout net\n"
            "cd cachecache/\n"
            "./clone_cachelib.sh\n"
            "./build.sh\n"
            "cd .build\n"
            "make -j12\n"

            "mkdir -p ~/execs/cache\n"
            "mkdir ~/execs/cache/libs\n"
            "mv ~/cachecache/cachecache/CacheLib/opt/cachelib/lib/lib* ~/execs/cache/libs\n"
            "mv ~/cachecache/cachecache/.build/supervisor ~/execs/cache/\n"
            "mv ~/cachecache/cachecache/.build/cache ~/execs/cache/\n"
            "cd ~/execs/cache\n"
            "for i in $(ldd supervisor | grep \"not found\" | awk '{ print $1 }');  do  patchelf --add-needed \"./libs/${i}\" supervisor; done\n"
            "for i in $(ldd cache | grep \"not found\" | awk '{ print $1 }');  do  patchelf --add-needed \"./libs/${i}\" cache; done\n"

            "rm -rf ~/cachecache/"
            ;

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
            "sudo apt-get install sbt openjdk-18-jdk\n"
            "GIT_SSH_COMMAND=\"ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no\" git clone git@github.com:davidson-consulting/cachecache.git\n"
            "cd cachecache/\n"
            "git checkout net\n"
            "mkdir ~/gatling\n"
            "mv web-apps/socialNet/workload/ ~/gatling\n"
            "rm -rf cachecache/\n";

        auto r = m-> runScript (script);
        r-> wait ();
        LOG_INFO (r-> stdout ());
    }
    
}
