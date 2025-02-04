#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

#include <utils/mysql/client.hh>
#include <utils/cache/client.hh>

namespace socialNet::social_graph {

    struct Sub {
        uint32_t userId;
        uint32_t toWhom;
    };

    class SocialGraphDatabase {
    private:

        // The client connection to the DB
        std::shared_ptr <socialNet::utils::MysqlClient> _client;

        // The connection to the cache
        std::shared_ptr <socialNet::utils::CacheClient> _cache;

    public:

        /**
         * @params:
         *    - configPath: path to the configuration file
         */
        SocialGraphDatabase ();

        /**
         * Configure the database
         */
        void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & conf);

        /**
         * Insert a new post in the DB
         * @params:
         *    - post: the post to insert
         * @returns: the uniq id of the post
         */
        uint32_t insertSub (Sub & p);

        /**
         * Find a post from its uniq Id
         * @params:
         *   - p: the sub to find
         * @returns:
         *    - return: false iif not found
         */
        bool findSub (Sub p);

        /**
         * Find the subscriptions of a user
         * @params:
         *    - userId: the user
         *    - page: the page index
         *    - nbPerPage: the number of subs per page
         */

        std::shared_ptr <utils::MysqlClient::Statement> prepareFindSubscriptions (uint32_t * resId, uint32_t *uid, int32_t *page, int32_t *nb);
        std::shared_ptr <utils::MysqlClient::Statement> prepareFindFollowers (uint32_t * resId, uint32_t *uid, int32_t *page, int32_t *nb);

        std::vector <uint32_t> findFollowersCacheable (uint32_t uid, int32_t page, int32_t nb);
        std::vector <uint32_t> findSubsCacheable (uint32_t uid, int32_t page, int32_t nb);


        /**
         * Count the number of subscriptions of a user
         */
        uint32_t countNbSubs (uint32_t id);

        /**
         * Count the number of followers of a user
         */
        uint32_t countNbFollowers (uint32_t id);

        /**
         * Remove a subscription
         */
        void removeSub (Sub & sub);

        void dispose ();

    private:

        /**
         * Create the post and tags tables
         */
        void createTables ();

        bool findFollowersInCache (std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        void insertFollowersInCache (const std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        bool findSubsInCache (std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        void insertSubsInCache (const std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);

        bool hasCache () const;
    };

}
