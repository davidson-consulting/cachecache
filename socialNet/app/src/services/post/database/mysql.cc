#include "mysql.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::post {

    MysqlPostDatabase::MysqlPostDatabase () :
        _client (nullptr)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MysqlPostDatabase::configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) {
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

    void MysqlPostDatabase::createTables () {
        auto reqPost = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS post (\n"
                                                 "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                                 "user_id INT NOT NULL,\n"
                                                 "user_login VARCHAR (" + std::to_string (LOGIN_LEN) + ") NOT NULL,\n"
                                                 "text VARCHAR (" + std::to_string (TEXT_LEN) + ") NOT NULL\n)");
        reqPost-> finalize ();
        reqPost-> execute ();

        auto reqTags = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS tags (\n"
                                                 "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                                 "user_id INT NOT NULL,\n"
                                                 "post_id INT NOT NULL\n,"
                                                 "FOREIGN KEY (post_id) REFERENCES post(id)\n)");
        reqTags-> finalize ();
        reqTags-> execute ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          POSTS          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t MysqlPostDatabase::insertPost (uint32_t id, const std::string & userLogin, const std::string & text, uint32_t * tags, uint32_t nbTags) {
        Post p;
        ::memset (&p, 0, sizeof (Post));
        ::memcpy (p.userLogin, userLogin.c_str (), std::min (userLogin.length (), (uint64_t) (LOGIN_LEN - 1)));
        ::memcpy (p.text, text.c_str (), std::min (text.length (), (uint64_t) (TEXT_LEN - 1)));

        auto req = this-> _client-> prepare ("INSERT INTO post (user_id, user_login, text) values (?, ?, ?)");
        req-> setParam (0, &id);
        req-> setParam (1, p.userLogin, strnlen (p.userLogin, LOGIN_LEN));
        req-> setParam (2, p.text, strnlen (p.text, TEXT_LEN));

        req-> finalize ();
        req-> execute ();

        auto postId = req-> getGeneratedId ();
        req = nullptr;

        this-> insertTags (postId, tags, nbTags);
        return postId;
    }

    bool MysqlPostDatabase::findPost (uint32_t id, Post & p) {
        if (this-> findPostInCache (id, p)) return true;

        ::memset (&p, 0, sizeof (Post));
        auto req = this-> _client-> prepare ("SELECT user_id, user_login, text FROM post where id=?");
        req-> setParam (0, &id);
        req-> setResult (0, &p.userId);
        req-> setResult (1, p.userLogin, LOGIN_LEN);
        req-> setResult (2, p.text, TEXT_LEN);

        req-> finalize ();
        req-> execute ();
        if (req-> next ()) {
            req.reset ();
            this-> findTags (id, p.tags, p.nbTags);
            this-> insertPostInCache (id, p);

            return true;
        }

        return false;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          TAGS          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MysqlPostDatabase::insertTags (uint32_t postId, uint32_t * tags, uint8_t nbTags) {
        auto req = this-> _client-> prepare ("INSERT INTO tags (post_id, user_id) values (?, ?)");
        uint32_t tagId;
        req-> setParam (0, &postId);
        req-> setParam (1, &tagId);
        req-> finalize ();

        for (uint32_t i = 0 ; i < nbTags && i < MAX_TAGS; i++) {
            tagId = tags [i];
            req-> execute ();
        }
    }

    void MysqlPostDatabase::findTags (uint32_t postId, uint32_t * tags, uint8_t & nbTags) {
        auto req = this-> _client-> prepare ("SELECT user_id FROM tags where post_id=?");
        uint32_t tagId;

        req-> setParam (0, &postId);
        req-> setResult (0, &tagId);
        req-> finalize ();

        nbTags = 0;
        req-> execute ();
        while (req-> next () && nbTags < MAX_TAGS) {
            tags [nbTags] = tagId;
            nbTags += 1;
        }
    }

    void MysqlPostDatabase::dispose () {
        PostDatabase::dispose (); // dispose cache handle
        this-> _client-> dispose ();
        this-> _client.reset ();
    }

}
