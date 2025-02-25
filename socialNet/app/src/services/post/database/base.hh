#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <vector>

#include <utils/cache/client.hh>
#include <rd_utils/memory/cache/_.hh>

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

                // The connection to the cache
                std::shared_ptr <socialNet::utils::CacheClient> _cache;

        public:

                /**
                 * @params:
                 *    - configPath: path to the configuration file
                 */
                PostDatabase ();

                /**
                 * Configure the database
                 */
                virtual void configure (const std::string & dbName, const std::string & chName, const rd_utils::utils::config::ConfigNode & configPath) = 0;

                /**
                 * Insert a new post in the DB
                 * @params:
                 *    - post: the post to insert
                 * @returns: the uniq id of the post
                 */
                virtual uint32_t insertPost (uint32_t id, const std::string & login, const std::string & message, uint32_t * tags, uint32_t nbTags) = 0;

                /**
                 * Find a post from its uniq Id
                 * @params:
                 *    - postId: the id of the post
                 * @returns:
                 *    - p: the found post (if returning true, unchanged otherwise)
                 *    - return: false iif not found
                 */
                virtual bool findPost (uint32_t postId, Post & p) = 0;

                /**
                 * Dispose the database handles
                 */
                virtual void dispose ();

        protected:

                /**
                 * Configure the cache of the db
                 */
                void configureCache (const std::string & ch, const rd_utils::utils::config::ConfigNode & cfg);

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
