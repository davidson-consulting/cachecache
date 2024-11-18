#pragma once

#include <rd_utils/_.hh>
#include <string>
#include <memory>
#include <set>

namespace deployer {

        class Machine {

                // The host name of the machine (used for ssh connection)
                std::string _hostname;

                // THe user accessing the machine
                std::string _user;

                // The flags of the machine (used for install, and deploy filtering)
                std::set <std::string> _flags;

                // The iface accessible from internet
                std::string _iface;

                // The ip of the machine (discovered from iface)
                std::string _ip;

        public:

                /**
                 * @params:
                 *    - hostname: the name of the host machine
                 *    - user: the user of the machine
                 */
                Machine (const std::string & hostname, const std::string & user, const std::string & iface);

                /**
                 * Add a flag to the machine to filter installation
                 */
                void addFlag (const std::string & flg);

                /**
                 * @returns: true if the machine has a certain flag
                 */
                bool hasFlag (const std::string & flg);

                /**
                 * @returns: the name of the machine
                 */
                const std::string & getName () const;

                /**
                 * @returns: the remote iface of the machine accessible from anywhere
                 */
                const std::string & getIface () const;

                /**
                 * @returns: the remote ip of the machine accessible from anywhere
                 */
                const std::string & getIp () const;

                /**
                 * Run a single command on the remote node
                 * @params:
                 *    - cmd: the command to run
                 *    - where: the directory in which the command is run (by default /home/{user} if where == ".", or /root if user is root)
                 */
                std::shared_ptr <rd_utils::concurrency::SSHProcess> run (const std::string & cmd, const std::string & where = ".");

                /**
                 * Run a complex bash script on remote node
                 * @params:
                 *   - script: the content of the script to run
                 */
                std::shared_ptr <rd_utils::concurrency::SSHProcess> runScript (const std::string & script);

                /**
                 * Copy a string to a remote file
                 * @params:
                 *    - content: the content of the file to write
                 *    - where: the directory in which to put the file
                 */
                void putFromStr (const std::string & content, const std::string & where);

                /**
                 * Copy a file on the remote node
                 * @params:
                 *    - filePath: the path of the file on the local node
                 *    - where: the directory in which to put the file (by default /home/{user} if where == ".", or /root if user is root)
                 */
                void put (const std::string & filePath, const std::string & where = ".");

                /**
                 * Download a file from remote node
                 * @params:
                 *    - filePath: the path of the file on the remote node
                 *    - where: the directory in which to put the file (by default /tmp/{hostname} if where == "", or /root if user is root)
                 */
                void get (const std::string & filePath, const std::string & where = "");

        private:

                void discoverIP ();

        };
    
}
