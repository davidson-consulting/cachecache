#include "mysql.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::timeline {

    uint32_t MysqlTimelineDatabase::BUFFER_MAX = 8192;

    MysqlTimelineDatabase::MysqlTimelineDatabase () :
        _client (nullptr)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MysqlTimelineDatabase::configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) {
        auto dbName = conf ["db"][db].getOr ("name", "socialNet");
        auto dbAddr = conf ["db"][db].getOr ("addr", "localhost");
        auto dbUser = conf ["db"][db].getOr ("user", "root");
        auto dbPass = conf ["db"][db].getOr ("pass", "root");
        auto dbPort = conf ["db"][db].getOr ("port", 0);

        this-> _client = std::make_shared <socialNet::utils::MysqlClient> (dbAddr, dbUser, dbPass, dbName, dbPort);
        this-> _client-> connect ();
        this-> createTables ();
        this-> configureCache (ch, conf);
    }

    void MysqlTimelineDatabase::createTables () {
        auto reqTimeline = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS home_timeline (\n"
                                                     "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                                     "user_id INT NOT NULL,\n"
                                                     "post_id INT NOT NULL\n)"
                                                     );
        reqTimeline-> finalize ();
        reqTimeline-> execute ();

        auto reqPostTimeline = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS post_timeline (\n"
                                                         "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                                         "user_id INT NOT NULL,\n"
                                                         "post_id INT NOT NULL\n)"
                                                         );
        reqPostTimeline-> finalize ();
        reqPostTimeline-> execute ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ==================================          INSERT/FINDS          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MysqlTimelineDatabase::insertHome (uint32_t uid, uint32_t pid) {
        WITH_LOCK (this-> _m) {
            this-> _bufferHome.push_back (Time {.uid = uid, .pid = pid});
            this-> _bufferSize += 1;
        }

        if (this-> _bufferSize > BUFFER_MAX) this-> commit ();
    }

    void MysqlTimelineDatabase::insertPost (uint32_t uid, uint32_t pid) {
        WITH_LOCK (this-> _m) {
            this-> _bufferPost.push_back (Time {.uid = uid, .pid = pid});
            this-> _bufferSize += 1;
        }

        if (this-> _bufferSize > BUFFER_MAX) this-> commit ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          FINDING          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::vector <uint32_t> MysqlTimelineDatabase::findHome (uint32_t uid, uint32_t page, uint32_t nb) {
        std::vector <uint32_t> result;
        result.reserve (nb);

        if (this-> findHomeInCache (result, uid, page, nb)) return result;

        uint32_t pid;
        uint32_t offset = page * nb;
        auto statement = this-> prepareFindHome (&pid, &uid, &offset, &nb);
        statement-> execute ();
        while (statement-> next ()) {
            result.push_back (pid);
        }

        this-> insertHomeInCache (result, uid, page, nb);
        return result;
    }

    std::vector <uint32_t> MysqlTimelineDatabase::findPost (uint32_t uid, uint32_t page, uint32_t nb) {
        std::vector <uint32_t> result;
        result.reserve (nb);

        if (this-> findPostInCache (result, uid, page, nb)) return result;

        uint32_t pid;
        uint32_t offset = page * nb;
        auto statement = this-> prepareFindPost (&pid, &uid, &offset, &nb);
        statement-> execute ();
        while (statement-> next ()) {
            result.push_back (pid);
        }

        this-> insertPostInCache (result, uid, page, nb);
        return result;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          PREPARE          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::shared_ptr <utils::MysqlClient::Statement> MysqlTimelineDatabase::prepareFindHome (uint32_t * pid, uint32_t * uid, uint32_t * page, uint32_t * nb) {
        auto req = this-> _client-> prepare ("SELECT post_id from home_timeline where user_id = ? order by id DESC limit ? offset ?");
        req-> setParam (0, uid);
        req-> setParam (1, nb);
        req-> setParam (2, page);
        req-> setResult (0, pid);
        req-> finalize ();

        return req;
    }

    std::shared_ptr <utils::MysqlClient::Statement> MysqlTimelineDatabase::prepareFindPost (uint32_t * pid, uint32_t * uid, uint32_t * page, uint32_t * nb) {
        auto req = this-> _client-> prepare ("SELECT post_id from post_timeline where user_id = ? order by id DESC limit ? offset ?");

        req-> setParam (0, uid);
        req-> setParam (1, nb);
        req-> setParam (2, page);
        req-> setResult (0, pid);
        req-> finalize ();

        return req;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          COUNTING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t MysqlTimelineDatabase::countPosts (uint32_t id) {
        auto req = this-> _client-> prepare ("SELECT COUNT(post_id) from post_timeline where user_id=?");
        req-> setParam (0, &id);

        uint32_t nb = 0;
        req-> setResult (0, &nb);
        req-> finalize ();
        req-> execute ();
        if (req-> next ()) {
            return nb;
        }

        return 0;
    }

    uint32_t MysqlTimelineDatabase::countHome (uint32_t id) {
        auto req = this-> _client-> prepare ("SELECT COUNT(post_id) from home_timeline where user_id=?");
        req-> setParam (0, &id);

        uint32_t nb = 0;
        req-> setResult (0, &nb);
        req-> finalize ();
        req-> execute ();
        if (req-> next ()) {
            return nb;
        }

        return 0;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          DISPOSING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MysqlTimelineDatabase::dispose () {
        this-> _client-> dispose ();
        this-> _client.reset ();
        TimelineDatabase::dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          COMMIT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MysqlTimelineDatabase::commit () {
        WITH_LOCK (this-> _m) {
            if (this-> _bufferPost.size () != 0) {
                std::stringstream ss;
                ss << "INSERT INTO post_timeline (user_id, post_id) values ";
                for (uint32_t i = 0 ; i < this-> _bufferPost.size (); i++) {
                    if (i != 0) ss << ", ";
                    ss << "(?, ?)";
                }

                auto req = this-> _client-> prepare (ss.str ());
                for (uint32_t i = 0, j = 0 ; i < this-> _bufferPost.size () ; j += 2, i++) {
                    req-> setParam (j, &this-> _bufferPost [i].uid);
                    req-> setParam (j + 1, &this-> _bufferPost [i].pid);
                }

                req-> finalize ();
                req-> execute ();
            }

            if (this-> _bufferHome.size () != 0) {
                std::stringstream ss;
                ss << "INSERT INTO home_timeline (user_id, post_id) values ";
                for (uint32_t i = 0 ; i < this-> _bufferHome.size (); i++) {
                    if (i != 0) ss << ", ";
                    ss << "(?, ?)";
                }

                auto req = this-> _client-> prepare (ss.str ());
                for (uint32_t i = 0, j = 0 ; i < this-> _bufferHome.size () ; j += 2, i++) {
                    req-> setParam (j, &this-> _bufferHome [i].uid);
                    req-> setParam (j + 1, &this-> _bufferHome [i].pid);
                }

                req-> finalize ();
                req-> execute ();
            }

            this-> _bufferPost.clear ();
            this-> _bufferHome.clear ();
            this-> _bufferSize = 0;
        }

        this-> _client-> commit ();
    }

}
