#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

#include <utils/mysql/client.hh>
#include <utils/cache/client.hh>

#define MAX_TAGS 16
#define LOGIN_LEN 16
#define TEXT_LEN 560

namespace socialNet::post {

    struct Post {
        uint32_t userId;
        char userLogin [LOGIN_LEN];
        char text [TEXT_LEN];
        uint32_t tags [MAX_TAGS];
        uint8_t nbTags;
    };

    class PostDatabase {
    private:

        // The client connection to the DB
        std::shared_ptr <socialNet::utils::MysqlClient> _client;

        // The connection to the cache
        std::shared_ptr <socialNet::utils::CacheClient> _cache;

        uint64_t _hit = 0;
        uint64_t _fail = 0;

    public:

        /**
         * @params:
         *    - configPath: path to the configuration file
         */
        PostDatabase ();

        /**
         * Configure the database
         */
        void configure (const std::string & dbName, const std::string & chName, const rd_utils::utils::config::ConfigNode & configPath);

        /**
         * Insert a new post in the DB
         * @params:
         *    - post: the post to insert
         * @returns: the uniq id of the post
         */
        uint32_t insertPost (uint32_t id, const std::string & login, const std::string & message, uint32_t * tags, uint32_t nbTags);

        /**
         * Find a post from its uniq Id
         * @params:
         *    - postId: the id of the post
         * @returns:
         *    - p: the found post (if returning true, unchanged otherwise)
         *    - return: false iif not found
         */
        bool findPost (uint32_t postId, Post & p);


        void dispose ();

    private:

        /**
         * Find the tags of a post
         * @returns:
         *     - tags: filling the list
         *     - nbTags: the number of tags (max = 16)
         */
        void findTags (uint32_t postId, uint32_t * tags, uint8_t & nbTags);

        /**
         * Insert a list of user id tagged in a post
         * @params:
         *     - postId: the post in which there are tags
         *     - tags: the list of user id tags
         */
        void insertTags (uint32_t postId, uint32_t * tags, uint8_t nbTags);

        /**
         * Create the post and tags tables
         */
        void createTables ();

        /**
         * Find a post in the cache
         */
        bool findPostInCache (uint32_t id, Post & p);

        /**
         * Insert a post in the cache
         */
        void insertPostInCache (uint32_t id, Post & p);

    };

}

std::ostream& operator<< (std::ostream & s, const socialNet::post::Post & p);
