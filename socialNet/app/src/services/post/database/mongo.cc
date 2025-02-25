#include "mongo.hh"

namespace socialNet::post {

    MongoPostDatabase::MongoPostDatabase () {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MongoPostDatabase::configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) {
        auto dbName = conf ["db"][db].getOr ("name", "socialNet");
        auto dbAddr = conf ["db"][db].getOr ("addr", "localhost");
        auto dbPort = conf ["db"][db].getOr ("port", 6660);
        CONF_LET (user, conf ["db"][db]["user"].getStr (), std::string ("root"));
        CONF_LET (pass, conf ["db"][db]["pass"].getStr (), std::string ("root"));

        this-> _client = std::make_shared <socialNet::utils::MongoPool> (dbAddr, dbPort, dbName);
        this-> _client-> connect (user, pass);
        this-> createTables ();
        this-> configureCache (ch, conf);
    } 

    void MongoPostDatabase::createTables () {
        auto cl = this-> _client-> get ();
        cl.createIndex ("post", "post_id", true);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          POSTS          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t MongoPostDatabase::insertPost (uint32_t id, const std::string & userLogin, const std::string & text, uint32_t * tags, uint32_t nbTags) {
        throw std::runtime_error ("TODO");
    }

    bool MongoPostDatabase::findPost (uint32_t id, Post & p) {
        throw std::runtime_error ("TODO");
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          TAGS          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MongoPostDatabase::insertTags (uint32_t postId, uint32_t * tags, uint8_t nbTags) {
        throw std::runtime_error ("TODO");
    }

    void MongoPostDatabase::findTags (uint32_t postId, uint32_t * tags, uint8_t & nbTags) {
        throw std::runtime_error ("TODO");
    }


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          DISPOSING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MongoPostDatabase::dispose () {
        PostDatabase::dispose ();
        this-> _client-> dispose ();
        this-> _client.reset ();
    }

}
