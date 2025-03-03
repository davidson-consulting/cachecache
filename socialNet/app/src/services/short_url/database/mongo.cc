#include "mongo.hh"
#include <nlohmann/json.hpp>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::short_url {

  MongoShortUrlDatabase::MongoShortUrlDatabase () :
    _client (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MongoShortUrlDatabase::configure (const std::string & db, const std::string & ch, const config::ConfigNode & conf) {
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

  void MongoShortUrlDatabase::createTables () {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================              SH_URL             =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MongoShortUrlDatabase::insertUrl (ShortUrl & p) {
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("short_url");

    bson_t * new_doc = bson_new ();
    BSON_APPEND_UTF8 (new_doc, "sh", p.sh);
    BSON_APPEND_UTF8 (new_doc, "ln", p.ln);

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

  bool MongoShortUrlDatabase::findUrl (ShortUrl & p) {
    if (this-> findUrlFromLongInCache (p)) return true;

    ::memset (p.sh, 0, SHORT_LEN);
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("short_url");

    bson_t * query = bson_new ();
    BSON_APPEND_UTF8 (query, "ln", p.ln);

    mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (coll, query, nullptr, nullptr);
    const bson_t * doc;
    bool found = mongoc_cursor_next (cursor, &doc);

    if (found) {
      auto post_json_char = bson_as_json (doc, nullptr);
      auto post_json = nlohmann::json::parse (post_json_char);

      std::string sh = post_json ["sh"];
      ::memcpy (p.sh, sh.c_str (), std::min (sh.length (), (uint64_t) (SHORT_LEN - 1)));
    }

    bson_destroy (query);
    mongoc_cursor_destroy (cursor);
    mongoc_collection_destroy (coll);

    return found;
  }

  bool MongoShortUrlDatabase::findUrlFromShort (ShortUrl & p) {
    if (this-> findUrlFromShort (p)) return true;

    ::memset (p.ln, 0, LONG_LEN);
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("short_url");

    bson_t * query = bson_new ();
    BSON_APPEND_UTF8 (query, "sh", p.sh);

    mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (coll, query, nullptr, nullptr);
    const bson_t * doc;
    bool found = mongoc_cursor_next (cursor, &doc);

    if (found) {
      auto post_json_char = bson_as_json (doc, nullptr);
      auto post_json = nlohmann::json::parse (post_json_char);

      std::string ln = post_json ["ln"];
      ::memcpy (p.ln, ln.c_str (), std::min (ln.length (), (uint64_t) (LONG_LEN - 1)));
    }

    bson_destroy (query);
    mongoc_cursor_destroy (cursor);
    mongoc_collection_destroy (coll);

    return found;
  }

  void MongoShortUrlDatabase::dispose () {
    this-> _client-> dispose ();
    this-> _client.reset ();
    ShortUrlDatabase::dispose ();
  }

}
