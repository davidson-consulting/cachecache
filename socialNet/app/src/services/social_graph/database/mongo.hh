#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

#include <utils/mongo/client.hh>
#include "base.hh"

namespace socialNet::social_graph {

        class MongoSocialGraphDatabase : public SocialGraphDatabase {
        private:

                // The client connection to the DB
                std::shared_ptr <socialNet::utils::MongoPool> _client;

        public:

                /**
                 * @params:
                 *    - configPath: path to the configuration file
                 */
                MongoSocialGraphDatabase ();

                /**
                 * Configure the database
                 */
                void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) override;

                /**
                 * Insert a new post in the DB
                 * @params:
                 *    - post: the post to insert
                 * @returns: the uniq id of the post
                 */
                void insertSub (Sub & p) override;


                /**
                 * @returns: true if the user is subscribed
                 */
                bool findSub (Sub s) override;

                /**
                 * Find the subscriptions of a user
                 * @params:
                 *    - userId: the user
                 *    - page: the page index
                 *    - nbPerPage: the number of subs per page
                 */
                std::vector <uint32_t> findFollowers (uint32_t uid, uint32_t page, uint32_t nb) override;
                std::vector <uint32_t> findSubs (uint32_t uid, uint32_t page, uint32_t nb) override;

                /**
                 * Count the number of subscriptions of a user
                 */
                uint32_t countNbSubs (uint32_t id) override;

                /**
                 * Count the number of followers of a user
                 */
                uint32_t countNbFollowers (uint32_t id) override;

                /**
                 * Remove a subscription
                 */
                void removeSub (Sub sub) override;


                void dispose () override;

        private:

                /**
                 * Create the post and tags tables
                 */
                void createTables ();

        };

}
