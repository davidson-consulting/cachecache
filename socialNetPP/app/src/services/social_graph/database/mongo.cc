#include "mongo.hh"
#include <rd_utils/_.hh>
#include <nlohmann/json.hpp>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::social_graph {

  MongoSocialGraphDatabase::MongoSocialGraphDatabase () :
    _client (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MongoSocialGraphDatabase::configure (const std::string & db, const std::string & ch, const config::ConfigNode & conf) {
    auto dbName = conf ["db"][db].getOr ("name", "socialNet");
    auto dbAddr = conf ["db"][db].getOr ("addr", "localhost");
    auto dbPort = conf ["db"][db].getOr ("port", 6660);
    CONF_LET (user, conf ["db"][db]["user"].getStr (), std::string ("root"));
    CONF_LET (pass, conf ["db"][db]["pass"].getStr (), std::string ("root"));

    this-> _client = std::make_shared <socialNet::utils::MongoPool> (dbAddr, dbPort, dbName);
    this-> _client-> connect (user, pass);

    this-> configureCache (ch, conf);
  }

  void MongoSocialGraphDatabase::createTables () {
  }

  void MongoSocialGraphDatabase::dispose () {
    this-> _client-> dispose ();
    this-> _client.reset ();
    SocialGraphDatabase::dispose ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERT/FIND/RM          =================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MongoSocialGraphDatabase::insertSub (Sub & p) {
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("social_graph");

    bson_t * new_doc = bson_new ();
    BSON_APPEND_INT32 (new_doc, "user_id", p.userId);
    BSON_APPEND_INT32 (new_doc, "to_whom", p.toWhom);

    bson_error_t error;
    bool inserted = mongoc_collection_insert_one (coll, new_doc, nullptr, nullptr, &error);
    if (!inserted) {
      std::runtime_error err ("Failed to insert in DB => " + std::string (error.message));

      bson_destroy(new_doc);
      mongoc_collection_destroy(coll);
      throw err;
    }

    bson_destroy (new_doc);
    mongoc_collection_destroy (coll);
  }

  bool MongoSocialGraphDatabase::findSub (Sub s) {
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("social_graph");

    bson_t * query = bson_new ();
    BSON_APPEND_INT32 (query, "user_id", s.userId);
    BSON_APPEND_INT32 (query, "to_whom", s.toWhom);
    mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (coll, query, nullptr, nullptr);

    const bson_t * doc;
    bool found = mongoc_cursor_next (cursor, &doc);

    bson_destroy (query);
    mongoc_cursor_destroy (cursor);
    mongoc_collection_destroy (coll);

    return found;
  }

  void MongoSocialGraphDatabase::removeSub (Sub s) {
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("social_graph");

    bson_t * new_doc = bson_new ();
    BSON_APPEND_INT32 (new_doc, "user_id", s.userId);
    BSON_APPEND_INT32 (new_doc, "to_whom", s.toWhom);

    bson_error_t error;
    bool deleted = mongoc_collection_delete_one (coll, new_doc, nullptr, nullptr, &error);
    if (!deleted) {
      std::runtime_error err ("Failed to insert in DB => " + std::string (error.message));

      bson_destroy(new_doc);
      mongoc_collection_destroy(coll);
      throw err;
    }

    bson_destroy (new_doc);
    mongoc_collection_destroy (coll);
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          FIND LIST          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::vector <uint32_t> MongoSocialGraphDatabase::findFollowers (uint32_t uid, uint32_t page, uint32_t nb) {
    std::vector <uint32_t> result;
    result.reserve (nb);
    if (this-> findFollowersInCache (result, uid, page, nb)) return result;

    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("social_graph");

    auto query = BCON_NEW ("filter",
                           "{",
                           "to_whom",
                           BCON_INT32 (uid),
                           "}");

    uint32_t offset = page * nb;
    auto cursor = mongoc_collection_find (coll, MONGOC_QUERY_NONE, offset, nb, 0, query, NULL, NULL);

    const bson_t* doc;
    while (mongoc_cursor_next (cursor, &doc)) {
      auto json_char = bson_as_json (doc, nullptr);
      auto js = nlohmann::json::parse (json_char);

      uint32_t id = js ["user_id"];
      result.push_back (id);
    }

    if (result.size () != 0) {
      this-> insertFollowersInCache (result, uid, page, nb);
    }

    bson_destroy (query);
    mongoc_cursor_destroy (cursor);
    mongoc_collection_destroy (coll);

    return result;
  }

  std::vector <uint32_t> MongoSocialGraphDatabase::findSubs (uint32_t uid, uint32_t page, uint32_t nb) {
    std::vector <uint32_t> result;
    result.reserve (nb);
    if (this-> findSubsInCache (result, uid, page, nb)) return result;

    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("social_graph");

    auto query = BCON_NEW ("filter",
                           "{",
                           "user_id",
                           BCON_INT32 (uid),
                           "}");

    uint32_t offset = page * nb;
    auto cursor = mongoc_collection_find (coll, MONGOC_QUERY_NONE, offset, nb, 0, query, NULL, NULL);

    const bson_t* doc;
    while (mongoc_cursor_next (cursor, &doc)) {
      auto json_char = bson_as_json (doc, nullptr);
      auto js = nlohmann::json::parse (json_char);

      uint32_t id = js ["to_whom"];
      result.push_back (id);
    }

    if (result.size () != 0) {
      this-> insertSubsInCache (result, uid, page, nb);
    }

    bson_destroy (query);
    mongoc_cursor_destroy (cursor);
    mongoc_collection_destroy (coll);

    return result;
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          COUNT          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */


  uint32_t MongoSocialGraphDatabase::countNbSubs (uint32_t uid) {
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("social_graph");

    bson_t * query = bson_new ();
    BSON_APPEND_INT32 (query, "user_id", uid);

    bson_error_t error;
    int64_t result = mongoc_collection_count_documents (coll, query, nullptr, nullptr, nullptr, &error);
    if (result == -1) {
      LOG_ERROR ("Failed to read number of subscriptions");
    }

    bson_destroy (query);
    mongoc_collection_destroy (coll);

    return result;
  }

  uint32_t MongoSocialGraphDatabase::countNbFollowers (uint32_t uid) {
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("social_graph");

    bson_t * query = bson_new ();
    BSON_APPEND_INT32 (query, "to_whom", uid);

    bson_error_t error;
    int64_t result = mongoc_collection_count_documents (coll, query, nullptr, nullptr, nullptr, &error);
    if (result == -1) {
      LOG_ERROR ("Failed to read number of followers");
    }

    bson_destroy (query);
    mongoc_collection_destroy (coll);

    return result;
  }


}
