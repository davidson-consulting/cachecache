#include "client.hh"
#include <stdexcept>

namespace socialNet::utils {

    MongoPool::MongoPool (const std::string & addr, uint32_t port, const std::string & appName)
        : _addr (addr)
        , _port (port)
        , _appName (appName)
    {}

    void MongoPool::connect (const std::string & user, const std::string & password, uint32_t timeout, uint32_t maxPoolSize) {
        std::string uri = "mongodb://" + user + ":" + password + "@" + this-> _addr + ":" + std::to_string (this-> _port) + "/?appname=" + this-> _appName;
        uri += "&" MONGOC_URI_SERVERSELECTIONTIMEOUTMS "=" + std::to_string (timeout * 1000);

        mongoc_init ();
        bson_error_t error;
        this-> _uri = mongoc_uri_new_with_error (uri.c_str (), &error);
        if (this-> _uri == nullptr) {
            throw std::runtime_error ("Failed to parse mongodb uri : (" + uri + ") => " + error.message);
        }

        this-> _pool = mongoc_client_pool_new (this-> _uri);
        mongoc_client_pool_max_size (this-> _pool, maxPoolSize);
    }

    MongoClient MongoPool::get () {
        return MongoClient (this, mongoc_client_pool_pop (this-> _pool));
    }

    void MongoPool::free (mongoc_client_t * cl) {
        mongoc_client_pool_push (this-> _pool, cl);
    }

    void MongoPool::dispose () {
        if (this-> _pool != nullptr) {
            mongoc_client_pool_destroy (this-> _pool);
            this-> _pool = nullptr;
        }

        if (this-> _uri != nullptr) {
            mongoc_uri_destroy (this-> _uri);
            this-> _uri = nullptr;
        }
    }

    MongoPool::~MongoPool () {
        this-> dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          CLIENT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */


    MongoClient::MongoClient (MongoPool * context, mongoc_client_t* cl)
        :  _context (context)
        , _conn (cl)
    {}

    mongoc_client_t* MongoClient::get () {
        return this-> _conn;
    }

    void MongoClient::createIndex (const std::string & collection, const std::string & name, bool uniq) {
        bson_t keys;
        mongoc_database_t * db = mongoc_client_get_database (this-> _conn, collection.c_str ());
        bson_init (&keys);

        BSON_APPEND_INT32 (&keys, name.c_str (), 1);
        char * index_name = mongoc_collection_keys_to_index_string (&keys);
        bson_t* create_indexes = BCON_NEW (
                                           "createIndexes", BCON_UTF8 (collection.c_str ()),
                                           "indexes", "[", "{",
                                           "key", BCON_DOCUMENT (&keys),
                                           "name", BCON_UTF8 (index_name),
                                           "unique", BCON_BOOL (uniq),
                                           "}", "]");

        bson_t reply;
        bson_error_t error;
        bool r = mongoc_database_write_command_with_opts (db, create_indexes, nullptr, &reply, &error);

        bson_free (index_name);
        bson_destroy (&reply);
        bson_destroy (create_indexes);
        mongoc_database_destroy (db);

        if (!r) {
            throw std::runtime_error ("Failed to create index : " + collection + " (" + name + ") => " + error.message);
        }
    }

    void MongoClient::dispose () {
        if (this-> _conn != nullptr) {
            this-> _context-> free (this-> _conn);
            this-> _conn = nullptr;
            this-> _context = nullptr;
        }
    }

    MongoClient::~MongoClient () {
        this-> dispose ();
    }
}
