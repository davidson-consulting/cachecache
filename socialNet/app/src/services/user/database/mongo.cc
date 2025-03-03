#include "mongo.hh"
#include <rd_utils/_.hh>
#include <nlohmann/json.hpp>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::user {

  MongoUserDatabase::MongoUserDatabase (uint32_t uniqId) :
    _client (nullptr)
    , _machineId (uniqId * 1000000)
    , _counter (0)
  {}

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          CONFIGURATION          ==================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MongoUserDatabase::configure (const std::string & db, const std::string & ch, const config::ConfigNode & conf) {
    auto dbName = conf ["db"][db].getOr ("name", "socialNet");
    auto dbAddr = conf ["db"][db].getOr ("addr", "localhost");
    auto dbPort = conf ["db"][db].getOr ("port", 6660);
    CONF_LET (user, conf ["db"][db]["user"].getStr (), std::string ("root"));
    CONF_LET (pass, conf ["db"][db]["pass"].getStr (), std::string ("root"));

    this-> _client = std::make_shared <socialNet::utils::MongoPool> (dbAddr, dbPort, dbName);
    this-> _client-> connect (user, pass);
    this-> configureCache (ch, conf);
  }

  void MongoUserDatabase::createTables () {
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          INSERT/FIND          ===================================
   * ====================================================================================================
   * ====================================================================================================
   */

  uint32_t MongoUserDatabase::insertUser (User & p) {
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("user");

    this-> _counter += 1;
    uint64_t uniqId = this-> _machineId + this-> _counter;
    bson_t * new_doc = bson_new ();
    BSON_APPEND_INT32 (new_doc, "user_id", uniqId);
    BSON_APPEND_UTF8 (new_doc, "login", p.login);
    BSON_APPEND_UTF8 (new_doc, "password", p.password);

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

  bool MongoUserDatabase::findByLogin (const std::string & login, User & u) {
    if (this-> findByLoginInCache (login, u)) return true;

    ::memset (&u, 0, sizeof (User));
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("user");

    bson_t * query = bson_new ();
    BSON_APPEND_UTF8 (query, "login", login.c_str ());

    mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (coll, query, nullptr, nullptr);

    const bson_t * doc ;
    bool found = mongoc_cursor_next (cursor, &doc);

    if (found) {
      auto post_json_char = bson_as_json (doc, nullptr);
      auto post_json = nlohmann::json::parse (post_json_char);

      u.id = post_json ["user_id"];
      std::string userLogin = post_json ["login"];
      std::string password = post_json ["password"];

      ::memcpy (u.login, userLogin.c_str (), std::min (userLogin.length (), (uint64_t) (LOGIN_LEN - 1)));
      ::memcpy (u.password, password.c_str (), std::min (password.length (), (uint64_t) (PASSWORD_LEN - 1)));
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(coll);

    return found;
  }

  bool MongoUserDatabase::findById (uint32_t & id, User & u) {
    if (this-> findByIdInCache (id, u)) return true;

    ::memset (&u, 0, sizeof (User));
    auto cl = this-> _client-> get ();
    auto coll = cl.getCollection ("user");

    bson_t * query = bson_new ();
    BSON_APPEND_INT32 (query, "user_id", id);

    mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (coll, query, nullptr, nullptr);

    const bson_t * doc ;
    bool found = mongoc_cursor_next (cursor, &doc);

    if (found) {
      auto post_json_char = bson_as_json (doc, nullptr);
      auto post_json = nlohmann::json::parse (post_json_char);

      u.id = post_json ["user_id"];
      std::string userLogin = post_json ["login"];
      std::string password = post_json ["password"];

      ::memcpy (u.login, userLogin.c_str (), std::min (userLogin.length (), (uint64_t) (LOGIN_LEN - 1)));
      ::memcpy (u.password, password.c_str (), std::min (password.length (), (uint64_t) (PASSWORD_LEN - 1)));
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(coll);

    return found;
  }

  void MongoUserDatabase::dispose () {
    this-> _client-> dispose ();
    this-> _client.reset ();
    UserDatabase::dispose ();
  }

}
