#pragma once

#include <cstdint>
#include <mongoc.h>
#include <bson/bson.h>
#include <string>

namespace socialNet::utils {

    class MongoPool;
    class MongoClient {
    private:
        MongoPool * _context;
        mongoc_client_t * _conn;

    public:

        MongoClient (MongoPool * context, mongoc_client_t * cl);
        MongoClient () = delete;
        MongoClient (const MongoClient &) = delete;
        void operator=(const MongoClient &) = delete;


        void createIndex (const std::string & collection, const std::string & name, bool uniq);
        mongoc_client_t * get ();
        void dispose ();

        ~MongoClient ();
    };

    class MongoPool {

        std::string _addr;
        uint32_t _port;
        std::string _appName;

        mongoc_client_pool_t * _pool;
        mongoc_uri_t * _uri;

    public :

        MongoPool (const std::string & addr, uint32_t port, const std::string & appName);

        void connect (const std::string & user, const std::string & password, uint32_t timeout = 2, uint32_t maxPoolSize = 10);

        /**
         * Take a client from the pool
         */
        MongoClient get ();

        void dispose ();

        /**
         * this-> dispose ();
         */
        ~MongoPool ();

    private :

        friend MongoClient;

        /**
         * Free the client in the pool
         */
        void free (mongoc_client_t * cl);

    };


}
