#pragma once

#include <rd_utils/_.hh>
#include <string>
#include <memory>

namespace deployer {

    class Machine {

        // The host name of the machine (used for ssh connection)
        std::string _hostname;

        // THe user accessing the machine
        std::string _user;

    public:

        /**
         * @params:
         *    - hostname: the name of the host machine
         *    - user: the user of the machine
         */
        Machine (const std::string & hostname, const std::string & user);

        /**
         * Run a list of commands on the remote node
         * @params:
         *    - cmd: the command to run
         *    - where: the directory in which the command is run (by default /home/{user} if where == ".", or /root if user is root)
         */
        std::shared_ptr <rd_utils::concurrency::SSHProcess> run (const std::string & cmd, const std::string & where = ".");

        /**
         * Copy a list of files on the remote node
         * @params:
         *    - filePath: the path of the file on the local node
         *    - where: the directory in which to put the file (by default /home/{user} if where == ".", or /root if user is root)
         */
        std::shared_ptr <rd_utils::concurrency::SCPProcess> put (const std::string & filePath, const std::string & where = ".");

        /**
         * Download a file from remote node
         * @params:
         *    - filePath: the path of the file on the remote node
         *    - where: the directory in which to put the file (by default /tmp/{hostname} if where == "", or /root if user is root)
         */
        std::shared_ptr <rd_utils::concurrency::SCPProcess> get (const std::string & filePath, const std::string & where = "");

    };
    
}
