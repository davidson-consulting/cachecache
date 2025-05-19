#include "db.hh"
#include "deployment.hh"
#include "cluster.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace deployer {

    DB::DB (Deployment * context, const std::string & name)
        : _context (context)
        , _name (name)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    const std::string & DB::getName () const {
        return this-> _name;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DB::configure (const std::string & cwd, const config::ConfigNode & cfg) {
        this-> _host = cfg ["host"].getStr ();
        this-> _kind = cfg.getOr ("kind", "mysql");
        this-> _base = cfg.getOr ("base", "");
        if (this-> _base != "") {
            this-> _base = rd_utils::utils::join_path (cwd, this-> _base);
        }


        if (this-> _kind == "mysql" || this-> _kind == "mongo") {
            this-> _port = cfg.getOr ("port", 3306);
        }
    }

    void DB::start () {
        if (this-> _kind == "mongo") {
            this-> deployMongo ();
        }

        else if (this-> _kind == "mysql") {
            this-> deployMysql ();
        }

        else {
            // Nothing to do to deploy file
        }
    }

    std::shared_ptr <config::ConfigNode> DB::createConfig () {
        auto host = this-> _context-> getCluster ()-> get (this-> _host);
        auto cfg = std::make_shared <config::Dict> ();
        cfg-> insert ("kind", this-> _kind);

        if (this-> _kind == "mysql") {
            cfg-> insert ("name", "socialNet");
            cfg-> insert ("addr", host-> getIp ());
            cfg-> insert ("user", "root");
            cfg-> insert ("pass", "");
            cfg-> insert ("port", (int64_t) this-> _port);
        }

        else if (this-> _kind == "mongo") {
            cfg-> insert ("name", "socialNet");
            cfg-> insert ("addr", host-> getIp ());
            cfg-> insert ("user", "root");
            cfg-> insert ("pass", "example");
            cfg-> insert ("port", (int64_t) this-> _port);
        }

        else {
            auto path = this-> dbPath ();
            cfg-> insert ("directory", path);
        }

        return cfg;
    }

    void DB::clean () {
        auto host = this-> _context-> getCluster ()-> get (this-> _host);
        auto path = this-> dbPath ();

        if (this-> _kind == "mongo") {
            auto script =
                "cd " + path + "/mongo\n"
                "docker-compose down\n";
            host-> run (script)-> wait ();
        }

        else if (this-> _kind == "mysql") {
            auto script =
                "cd " + path + "/mongo\n"
                "docker-compose down\n";
            host-> run (script)-> wait ();
        }

        LOG_INFO ("Remove db directory '" + this-> _name + "' on : ", this-> _host);
        host-> run ("rm -rf " + path)-> wait ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          RESTORING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DB::restoreBase () {
        if (this-> _kind == "mysql") {
            this-> installMysql ();
        }

        else if (this-> _kind == "mongo") {
            this-> installMongo ();
        }

        else {
            this-> installFile ();
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          MYSQL          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DB::deployMysql () {
        auto host = this-> _context-> getCluster ()-> get (this-> _host);
        auto path = this-> dbPath ();

        auto cmp =
            "version: '3.8'\n"
            "services:\n"
            "  mysql:\n"
            "    image: mysql:8.0.30\n"
            "    volumes:\n"
            "      - ./config/mysql:/etc/mysql/conf.d\n"
            "    environment:\n"
            "      - MYSQL_ALLOW_EMPTY_PASSWORD=yes\n"
            "    ports:\n"
            "      - " + std::to_string (this-> _port) + ":3306\n"
            "    command: mysqld --lower_case_table_names=1 --skip-ssl --character_set_server=utf8mb4 --explicit_defaults_for_timestamp\n";

        auto script =
            "cd " + path + "\n"
            "docker-compose up -d";

        host-> putFromStr (cmp, utils::join_path (path, "compose.yaml"));
        host-> runScript (script)-> wait ();
        this-> _timer.reset ();

        LOG_INFO ("Deploying DB on '", this-> _host);
        this-> _timer.sleep (1);

        host-> runAndWait ("mysql -e \"create database if not exists socialNet;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _port), 50, 0.2);
        LOG_INFO ("Created db on ", this-> _host, " (took : ", this-> _timer.time_since_start (), "s)");
    }

    void DB::installMysql () {
        auto host = this-> _context-> getCluster ()-> get (this-> _host);
        auto path = this-> dbPath ();

        host-> runAndWait ("mysql -e \"drop database if exists socialNet;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _port), 50, 0.2);
        host-> runAndWait ("mysql -e \"create database if not exists socialNet;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _port), 50, 0.2);
        LOG_INFO ("Upload sql file ", this-> _base, " to '", this-> _host, "':", utils::join_path (path, "dump.sql"));
        host-> put (this-> _base, utils::join_path (path, "dump.sql"));

        host-> runAndWait ("cd " + path + "; mysql -e \"use socialNet; source dump.sql;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _port), 50, 0.2);
        LOG_INFO ("Recreated db on ", this-> _host, " (took : ", this-> _timer.time_since_start (), "s)");
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          MONGO          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DB::installMongo () {
        auto host = this-> _context-> getCluster ()-> get (this-> _host);
        auto path = this-> dbPath ();

        // host-> runAndWait ("mongosh -e \"drop database if exists socialNet;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _port), 50, 0.2);
        // host-> runAndWait ("mysql -e \"create database if not exists socialNet;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _port), 50, 0.2);
        // LOG_INFO ("Upload sql file ", this-> _base, " to '", this-> _host, "':", utils::join_path (path, "dump.sql"));
        // host-> put (this-> _base, utils::join_path (path, "dump.sql"));

        // host-> runAndWait ("cd " + path + "; mysql -e \"use socialNet; source dump.sql;\" -u root -h 127.0.0.1 --port " + std::to_string (this-> _port), 50, 0.2);
        // LOG_INFO ("Recreated db on ", this-> _host, " (took : ", this-> _timer.time_since_start (), "s)");
    }

    void DB::deployMongo () {
        auto host = this-> _context-> getCluster ()-> get (this-> _host);
        auto path = this-> dbPath ();

        auto cmp = "version: '3.1'"
            "services:\n"
            "  mongo:\n"
            "    image: mongo\n"
            "    restart: always\n"
            "    environment:\n"
            "      MONGO_INITDB_ROOT_USERNAME: \"root\"\n"
            "      MONGO_INITDB_ROOT_PASSWORD: \"\"\n"
            ""
            "  mongo-express:\n"
            "    image: mongo-express\n"
            "    restart: always\n"
            "    ports:\n"
            "    - " + std::to_string (this-> _port) + ":8081\n"
            "    environment:\n"
            "      ME_CONFIG_MONGODB_ADMINUSERNAME: \"root\"\n"
            "      ME_CONFIG_MONGODB_ADMINPASSWORD: \"root\"\n"
            "      ME_CONFIG_MONGODB_URL: mongodb://root:example@mongo:27017/\n"
            "      ME_CONFIG_BASICAUTH: false\n"
            ;

        auto script =
            "cd " + path + "\n"
            "docker-compose up -d";

        host-> putFromStr (cmp, utils::join_path (path + "\n", "compose.yaml"));
        host-> runScript (script)-> wait ();
        this-> _timer.reset ();

        LOG_INFO ("Deploying DB on '", this-> _host);
        this-> _timer.sleep (1);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          FILE          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void DB::installFile () {
        auto host = this-> _context-> getCluster ()-> get (this-> _host);
        auto path = this-> dbPath ();

        try {
            host-> run ("rm -rf " + path)-> wait ();
            host-> run ("mkdir -p " + path)-> wait ();
            std::cout << this-> _base << " " << utils::join_path (path, "dirs.zip") << std::endl;
            host-> put (this-> _base, utils::join_path (path, "dirs.zip"));
            this-> _timer.sleep (5);

            auto p = host-> run ("cd " + path + " ; unzip -o dirs.zip");
            p-> stdout (); // needs to flush channel
            p-> wait ();
        } catch (...) {
            std::cout << "Error?" << std::endl;
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::string DB::dbPath () {
        auto host = this-> _context-> getCluster ()-> get (this-> _host);
        auto path = utils::join_path (host-> getHomeDir (), "/run/dbs/" + this-> _name);
        if (!this-> _hasPath) {
            host-> run ("mkdir -p " + path)-> wait ();

            this-> _hasPath = true;
        }

        return path;
    }


}
