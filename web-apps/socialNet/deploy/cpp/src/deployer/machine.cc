#include "machine.hh"

using namespace rd_utils;
using namespace rd_utils::concurrency;

namespace deployer {


    Machine::Machine (const std::string & host, const std::string & user)
        : _hostname (host)
        , _user (user)
    {}

    std::shared_ptr <SSHProcess> Machine::run (const std::string & cmd, const std::string & where) {
        std::string realCmd = cmd;
        if (where != ".") {
            realCmd = std::string ("cd ") + where + ";" + cmd;
        }

        auto proc = std::make_shared <SSHProcess> (this-> _hostname, realCmd, this-> _user);
        proc-> start ();

        return proc;
    }

    std::shared_ptr <SCPProcess> Machine::put (const std::string & file, const std::string & where) {
        std::string realOutPath = "";
        if (where == "") {
            realOutPath = utils::join_path ("./", utils::get_filename (file));
        } else {
            realOutPath = utils::join_path (where, utils::get_filename (file));
        }

        auto proc = std::make_shared <SCPProcess> (this-> _hostname, file, realOutPath, false, this-> _user);
        proc-> upload ();

        return proc;
    }

    std::shared_ptr <SCPProcess> Machine::get (const std::string & file, const std::string & where) {
        std::string realOutPath = "";
        if (where == "") {
            realOutPath = utils::join_path ("/tmp/" + this-> _hostname, utils::get_filename (file));
        } else {
            realOutPath = utils::join_path (where, utils::get_filename (file));
        }

        auto proc = std::make_shared <SCPProcess> (this-> _hostname, file, realOutPath, false, this-> _user);
        proc-> download ();

        return proc;
    }

}
