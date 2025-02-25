#include "mongo.hh"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

namespace socialNet::post {

    MongoPostDatabase::MongoPostDatabase (uint32_t id)
        : _client (nullptr)
        , _machineId (id * 10000000)
        , _counter (0)
    {}

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
        cl.createIndex ("post_id", true);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          POSTS          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t MongoPostDatabase::insertPost (uint32_t id, const std::string & userLogin, const std::string & text, uint32_t * tags, uint32_t nbTags) {
        auto cl = this-> _client-> get ();
        auto coll = cl.getCollection ("post");

        this-> _counter += 1;
        uint64_t uniqId = this-> _machineId + this-> _counter;
        bson_t * new_doc = bson_new ();
        BSON_APPEND_INT64 (new_doc, "post_id", uniqId);
        BSON_APPEND_UTF8 (new_doc, "text", text.c_str ());
        BSON_APPEND_UTF8 (new_doc, "user_login", userLogin.c_str ());
        BSON_APPEND_INT64 (new_doc, "user_id", id);

        bson_t user_mention_list;
        char buf [16];
        const char * key;
        BSON_APPEND_ARRAY_BEGIN (new_doc, "tags", &user_mention_list);
        for (uint32_t i = 0 ; i < nbTags ; i++) {
            bson_uint32_to_string (i, &key, buf, sizeof (buf));
            bson_t doc;
            BSON_APPEND_DOCUMENT_BEGIN (&user_mention_list, key, &doc);
            BSON_APPEND_INT64 (&doc, "user_id", tags [i]);
            bson_append_document_end (&user_mention_list, &doc);
        }
        bson_append_array_end (new_doc, &user_mention_list);

        bson_error_t error;
        bool inserted = mongoc_collection_insert_one (coll, new_doc, nullptr, nullptr, &error);

        if (!inserted) {
            std::runtime_error err ("Failed to insert in DB => " + std::string (error.message));

            bson_destroy(new_doc);
            mongoc_collection_destroy(coll);
            throw err;
        }

        bson_destroy(new_doc);
        mongoc_collection_destroy(coll);
        return uniqId;
    }

    bool MongoPostDatabase::findPost (uint32_t id, Post & p) {
        if (this-> findPostInCache (id, p)) return true;

        ::memset (&p, 0, sizeof (Post));
        auto cl = this-> _client-> get ();
        auto coll = cl.getCollection ("post");

        bson_t * query = bson_new ();
        BSON_APPEND_INT64 (query, "post_id", id);

        mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (coll, query, nullptr, nullptr);

        const bson_t * doc ;
        bool found = mongoc_cursor_next (cursor, &doc);

        if (found) {
            auto post_json_char = bson_as_json (doc, nullptr);
            auto post_json = nlohmann::json::parse (post_json_char);

            p.userId = post_json ["user_id"];
            std::string userLogin = post_json ["user_login"];
            std::string text = post_json ["text"];

            ::memcpy (p.userLogin, userLogin.c_str (), std::min (userLogin.length (), (uint64_t) (LOGIN_LEN - 1)));
            ::memcpy (p.text, text.c_str (), std::min (text.length (), (uint64_t) (TEXT_LEN - 1)));

            uint32_t i = 0;
            for (auto & tag : post_json ["tags"]) {
                if (i >= MAX_TAGS) break;

                p.tags [i] = tag ["user_id"];
                p.nbTags += 1;
            }
        }

        bson_destroy(query);
        mongoc_cursor_destroy(cursor);
        mongoc_collection_destroy(coll);

        return found;
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
