#include "mongo.hh"
#include <stdio.h>
#include <rd_utils/utils/files.hh>
#include <nlohmann/json.hpp>

using namespace rd_utils;

namespace socialNet::timeline {

    uint32_t MongoTimelineDatabase::BUFFER_MAX = 8192;

    MongoTimelineDatabase::MongoTimelineDatabase () {}

    void MongoTimelineDatabase::configure (const std::string & db, const std::string &ch, const rd_utils::utils::config::ConfigNode & conf) {
        auto dbName = conf ["db"][db].getOr ("name", "socialNet");
        auto dbAddr = conf ["db"][db].getOr ("addr", "localhost");
        auto dbPort = conf ["db"][db].getOr ("port", 6660);
        CONF_LET (user, conf ["db"][db]["user"].getStr (), std::string ("root"));
        CONF_LET (pass, conf ["db"][db]["pass"].getStr (), std::string ("root"));

        this-> _client = std::make_shared <socialNet::utils::MongoPool> (dbAddr, dbPort, dbName);
        this-> _client-> connect (user, pass);

        this-> configureCache (ch, conf);
    }

    void MongoTimelineDatabase::dispose () {
        TimelineDatabase::dispose ();
        this-> _client-> dispose ();
        this-> _client.reset ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          INSERTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MongoTimelineDatabase::insertHome (uint32_t uid, uint32_t pid) {
        WITH_LOCK (this-> _m) {
            this-> _bufferHome.push_back (Time {.uid = uid, .pid = pid});
            this-> _bufferSize += 1;
        }

        if (this-> _bufferSize > BUFFER_MAX) this-> commit ();
    }

    void MongoTimelineDatabase::insertPost (uint32_t uid, uint32_t pid) {
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

    std::vector <uint32_t> MongoTimelineDatabase::findHome (uint32_t uid, uint32_t page, uint32_t nb) {
        std::vector <uint32_t> result;
        result.reserve (nb);

        if (this-> findHomeInCache (result, uid, page, nb)) return result;

        auto cl = this-> _client-> get ();
        auto coll = cl.getCollection ("home_timeline");

        auto query = BCON_NEW ("$query",
                               "{",
                               "user_id",
                               BCON_INT32 (uid),
                               "}",
                               "$orderby",
                               "{",
                               "post_id",
                               BCON_INT32 (-1),
                               "}");

        uint32_t offset = page * nb;
        auto cursor = mongoc_collection_find (coll, MONGOC_QUERY_NONE, offset, nb, 0, query, NULL, NULL);

        const bson_t* doc;
        while (mongoc_cursor_next (cursor, &doc)) {
            auto json_char = bson_as_json (doc, nullptr);
            auto js = nlohmann::json::parse (json_char);

            uint32_t id = js ["post_id"];
            result.push_back (id);
        }

        if (result.size () != 0) {
            this-> insertHomeInCache (result, uid, page, nb);
        }

        bson_destroy (query);
        mongoc_cursor_destroy (cursor);
        mongoc_collection_destroy (coll);

        return result;
    }

    std::vector <uint32_t> MongoTimelineDatabase::findPost (uint32_t uid, uint32_t page, uint32_t nb) {
        std::vector <uint32_t> result;
        result.reserve (nb);

        if (this-> findPostInCache (result, uid, page, nb)) return result;

        auto cl = this-> _client-> get ();
        auto coll = cl.getCollection ("post_timeline");

        auto query = BCON_NEW ("filter",
                               "{",
                               "user_id",
                               BCON_INT32 (uid),
                               "}",
                               "sort",
                               "{",
                               "post_id",
                               BCON_INT32 (-1),
                               "}");

        uint32_t offset = page * nb;
        auto cursor = mongoc_collection_find (coll, MONGOC_QUERY_NONE, offset, nb, 0, query, NULL, NULL);

        const bson_t* doc;
        while (mongoc_cursor_next (cursor, &doc)) {
            auto json_char = bson_as_json (doc, nullptr);
            auto js = nlohmann::json::parse (json_char);

            uint32_t id = js ["post_id"];
            result.push_back (id);
        }

        if (result.size () != 0) {
            this-> insertPostInCache (result, uid, page, nb);
        }

        bson_destroy (query);
        mongoc_cursor_destroy (cursor);
        mongoc_collection_destroy (coll);

        return result;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          COUNTING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t MongoTimelineDatabase::countPosts (uint32_t uid) {
        auto cl = this-> _client-> get ();
        auto coll = cl.getCollection ("post_timeline");

        bson_t * query = bson_new ();
        BSON_APPEND_INT32 (query, "user_id", uid);

        bson_error_t error;
        int64_t result = mongoc_collection_count_documents (coll, query, nullptr, nullptr, nullptr, &error);
        if (result == -1) {
            LOG_ERROR ("Failed to read number of post in user timeline");
        }

        bson_destroy (query);
        mongoc_collection_destroy (coll);

        return result;
    }

    uint32_t MongoTimelineDatabase::countHome (uint32_t uid) {
        auto cl = this-> _client-> get ();
        auto coll = cl.getCollection ("home_timeline");

        bson_t * query = bson_new ();
        BSON_APPEND_INT32 (query, "user_id", uid);

        bson_error_t error;
        int64_t result = mongoc_collection_count_documents (coll, query, nullptr, nullptr, nullptr, &error);
        if (result == -1) {
            LOG_ERROR ("Failed to read number of post in home timeline");
        }

        bson_destroy (query);
        mongoc_collection_destroy (coll);

        return result;
    }


    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          COMMIT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void MongoTimelineDatabase::commit () {
        WITH_LOCK (this-> _m) {
            if (this-> _bufferPost.size () != 0) {
                auto cl = this-> _client-> get ();
                auto coll = cl.getCollection ("post_timeline");
                this-> insertInCollection (coll, this-> _bufferPost);
                mongoc_collection_destroy (coll);
            }

            if (this-> _bufferHome.size () != 0) {
                auto cl = this-> _client-> get ();
                auto coll = cl.getCollection ("home_timeline");
                this-> insertInCollection (coll, this-> _bufferHome);
                mongoc_collection_destroy (coll);
            }

            this-> _bufferHome.clear ();
            this-> _bufferPost.clear ();
            this-> _bufferSize = 0;
        }
    }

    void MongoTimelineDatabase::insertInCollection (mongoc_collection_t * coll, const std::vector <Time> & elements) {
        uint32_t nbElemPerTurn = 8192;
        uint32_t rest = elements.size () % nbElemPerTurn;
        uint32_t nbTurns = elements.size () / nbElemPerTurn;

        bson_t** querys = new bson_t* [nbElemPerTurn];
        for (uint32_t i = 0 ; i < nbTurns ; i++) {
            this-> performInsert (coll, &elements [i * nbElemPerTurn], querys, nbElemPerTurn);
        }

        if (rest != 0) {
            this-> performInsert (coll, &elements [nbElemPerTurn * nbTurns], querys, rest);
        }

        delete[] querys;
    }

    void MongoTimelineDatabase::performInsert (mongoc_collection_t * coll, const Time * elements, bson_t ** querys, uint32_t nbElements) {
        for (uint32_t i = 0 ; i < nbElements ; i++) {
            querys [i] = bson_new ();
            BSON_APPEND_INT32 (querys [i], "user_id", elements [i].uid);
            BSON_APPEND_INT32 (querys [i], "post_id", elements [i].pid);
        }

        bson_error_t error;
        if (!mongoc_collection_insert_many (coll, (const bson_t **) querys, nbElements, nullptr, nullptr, &error)) {
            LOG_ERROR ("Failed to insert in timeline : ", error.message);
        }

        for (uint32_t i = 0 ; i < nbElements ; i++) {
            bson_destroy (querys [i]);
        }
    }

}
