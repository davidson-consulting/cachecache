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
                virtual void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) = 0;

                /**
                 * Insert a new post in the DB
                 * @params:
                 *    - post: the post to insert
                 * @returns: the uniq id of the post
                 */
                virtual void insertSub (Sub & p) = 0;

                /**
                 * Find a post from its uniq Id
                 * @params:
                 *   - p: the sub to find
                 * @returns:
                 *    - return: false iif not found
                 */
                virtual bool findSub (Sub p) = 0;

                /**
                 * Find the subscriptions of a user
                 * @params:
                 *    - userId: the user
                 *    - page: the page index
                 *    - nbPerPage: the number of subs per page
                 */

                virtual std::vector <uint32_t> findFollowers (uint32_t uid, uint32_t page, uint32_t nb) = 0;
                virtual std::vector <uint32_t> findSubs (uint32_t uid, uint32_t page, uint32_t nb) = 0;


                /**
                 * Count the number of subscriptions of a user
                 */
                virtual uint32_t countNbSubs (uint32_t id) = 0;

                /**
                 * Count the number of followers of a user
                 */
                virtual uint32_t countNbFollowers (uint32_t id) = 0;

                /**
                 * Remove a subscription
                 */
                virtual void removeSub (Sub sub) = 0;

                virtual void dispose ();

        protected:

                void configureCache (const std::string & ch, const rd_utils::utils::config::ConfigNode & conf);

                bool findFollowersInCache (std::vector <uint32_t> & result, uint32_t uid, uint32_t page, uint32_t nb);
                void insertFollowersInCache (const std::vector <uint32_t> & result, uint32_t uid, uint32_t page, uint32_t nb);

                bool findSubsInCache (std::vector <uint32_t> & result, uint32_t uid, uint32_t page, uint32_t nb);
                void insertSubsInCache (const std::vector <uint32_t> & result, uint32_t uid, uint32_t page, uint32_t nb);

        };

}
