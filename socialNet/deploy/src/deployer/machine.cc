#include "machine.hh"

using namespace rd_utils;
using namespace rd_utils::concurrency;

namespace deployer {

    Machine::Machine (const std::string & host, const std::string & user, const std::string & workDir, const std::string & iface, bool pdu)
        : _hostname (host)
        , _workingDir (workDir)
        , _user (user)
        , _iface (iface)
        , _hasPDU (pdu)
    {
        this-> discoverIP ();
    }


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GET/SET          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Machine::addFlag (const std::string & flg) {
        this-> _flags.emplace (flg);
    }

    bool Machine::hasFlag (const std::string & flg) {
        return this-> _flags.find (flg) != this-> _flags.end ();
    }

    const std::string & Machine::getName () const {
        return this-> _hostname;
    }

    const std::string & Machine::getIface () const {
        return this-> _iface;
    }

    const std::string & Machine::getIp () const {
        return this-> _ip;
    }

    std::string Machine::getHomeDir () const {
        std::string home = "";
        if (this-> _user == "root") {
            home = "/root";
        } else {
            home = "/home/" + this-> _user;
        }

        if (this-> _workingDir == "") return home;
        else {
            return utils::findAndReplaceAll (this-> _workingDir, "~", home);
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          SSH/SCP          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::shared_ptr <SSHProcess> Machine::run (const std::string & cmd, const std::string & where) {
        std::string realCmd = cmd;
        if (where != ".") {
            realCmd = std::string ("cd ") + where + ";" + cmd;
        }

        auto proc = std::make_shared <SSHProcess> (this-> _hostname, realCmd, this-> _user);
        proc-> start ();

        return proc;
    }

    void Machine::runAndWait (const std::string & cmd, uint32_t nbTries, float sleepBetweenTries) {
        for (uint32_t nb = 1 ;; nb ++) {
            auto p = this-> run (cmd)-> wait ();
            if (p == 0) break;
            else if (nb >= nbTries) throw std::runtime_error ("Fail to run command " + cmd);
            else {
                concurrency::timer::sleep (sleepBetweenTries);
            }
        }
    }

    std::shared_ptr <SSHProcess> Machine::runScript (const std::string & script) {
        auto tmpDir = rd_utils::utils::create_temp_dirname (this-> _hostname);
        auto tmpFile = utils::join_path (tmpDir, "script.sh");
        rd_utils::utils::write_file (tmpFile, script);

        this-> run ("mkdir " + tmpDir)-> wait ();
        this-> put (tmpFile, tmpFile);

        utils::remove (tmpFile);
        utils::remove (tmpDir);

        return this-> run ("bash " + tmpFile, ".");
    }

    void Machine::putFromStr (const std::string & content, const std::string & where) {
        auto tmpDir = rd_utils::utils::create_temp_dirname (this-> _hostname);
        auto tmpFile = utils::join_path (tmpDir, "content");
        rd_utils::utils::write_file (tmpFile, content);

        this-> put (tmpFile, where);

        utils::remove (tmpFile);
        utils::remove (tmpDir);
    }

    std::string Machine::getToStr (const std::string & file, uint32_t nbTries, float sleepBetweenTries) {
        auto tmpDir = rd_utils::utils::create_temp_dirname (this-> _hostname);
        auto tmpFile = utils::join_path (tmpDir, "result");

        for (uint32_t nb = 1 ;; nb ++) {
            try {
                this-> get (file, tmpFile);

                auto result = rd_utils::utils::read_file (tmpFile);
                utils::remove (tmpFile);
                utils::remove (tmpDir);

                return result;
            } catch (const std::runtime_error & err) {
                if (nb >= nbTries) {
                    utils::remove (tmpFile);
                    utils::remove (tmpDir);
                    throw err;
                }
                concurrency::timer::sleep (sleepBetweenTries);
            }
        }
    }

    void Machine::put (const std::string & file, const std::string & where) {
        std::string realOutPath = "";
        if (where == "") {
            realOutPath = utils::join_path ("./", utils::get_filename (file));
        } else {
            realOutPath = utils::join_path (where, utils::get_filename (file));
        }

        auto proc = std::make_shared <SCPProcess> (this-> _hostname, file, realOutPath, false, this-> _user);
        proc-> upload ();
    }

    void Machine::get (const std::string & file, const std::string & where) {
        std::string realOutPath = "";
        if (where == "") {
            realOutPath = utils::join_path ("/tmp/" + this-> _hostname, utils::get_filename (file));
        } else {
            realOutPath = where;
        }

        auto proc = std::make_shared <SCPProcess> (this-> _hostname, file, realOutPath, false, this-> _user);
        proc-> download ();
    }

    bool Machine::hasPDU () const {
        return this-> _hasPDU;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          PRIVATE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Machine::discoverIP () {
        auto p = this-> run ("ip a show dev " + this-> _iface + " |  awk -F ' *|:' '/inet /{print $3}'");
        p-> wait ();
        this-> _ip = p-> stdout ();
        auto index = this-> _ip.find ("/");
        if (index != std::string::npos) {
            this-> _ip = this-> _ip.substr (0, index);
        }

        LOG_INFO ("Remote machine '", this-> _hostname, "' configured with ip = ", this-> _ip, ", wd = ", this-> getHomeDir ());
    }
    
    
}
