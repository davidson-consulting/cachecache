#include "machine.hh"

using namespace rd_utils;
using namespace rd_utils::concurrency;

namespace deployer {

    Machine::Machine (const std::string & host, const std::string & user, const std::string & iface)
        : _hostname (host)
        , _user (user)
        , _iface (iface)
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
        if (this-> _user == "root") return "/root";
        else return "/home/" + this-> _user;
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

    std::shared_ptr <SSHProcess> Machine::runScript (const std::string & script) {
        auto tmpDir = rd_utils::utils::create_temp_dirname (this-> _hostname);
        auto tmpFile = utils::join_path (tmpDir, "script.sh");
        rd_utils::utils::write_file (tmpFile, script);

        this-> run ("mkdir " + tmpDir)-> wait ();
        this-> put (tmpFile, tmpFile);
        std::cout << tmpFile << std::endl;

        return this-> run ("bash " + tmpFile, ".");
    }

    void Machine::putFromStr (const std::string & content, const std::string & where) {
        auto tmpDir = rd_utils::utils::create_temp_dirname (this-> _hostname);
        auto tmpFile = utils::join_path (tmpDir, "content");
        rd_utils::utils::write_file (tmpFile, content);

        this-> put (tmpFile, where);
    }

    std::string Machine::getToStr (const std::string & file) {
        auto tmpDir = rd_utils::utils::create_temp_dirname (this-> _hostname);
        auto tmpFile = utils::join_path (tmpDir, "result");
        this-> get (file, tmpFile);

        return rd_utils::utils::read_file (tmpFile);
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

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          PRIVATE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Machine::discoverIP () {
        auto p = this-> run ("ip a show dev eth0 |  awk -F ' *|:' '/inet /{print $3}'");
        p-> wait ();
        this-> _ip = p-> stdout ();
        auto index = this-> _ip.find ("/");
        if (index != std::string::npos) {
            this-> _ip = this-> _ip.substr (0, index);
        }

        LOG_INFO ("Ip of machine : ", this-> _hostname, " is ", this-> _ip);
    }
    
    
}
