#pragma once

#include <rd_utils/_.hh>

namespace deployer {

    class Deployment;
    class Machine;

    class DB {
    private:

        Deployment * _context;

        std::string _name;
        std::string _kind;
        std::string _base;

        std::string _host;
        uint32_t _port;

        rd_utils::concurrency::timer _timer;
        bool _hasPath = false;

    public :

        DB (Deployment * context, const std::string & name);

        void configure (const rd_utils::utils::config::ConfigNode & cfg);

        std::shared_ptr <rd_utils::utils::config::ConfigNode> createConfig ();

        void restoreBase ();

        void start ();

        void clean ();


        const std::string & getName () const;

    private:

        void deployMongo ();

        void deployMysql ();

        void installMysql ();

        void installMongo ();

        void installFile ();

        std::string dbPath ();

    };


}
