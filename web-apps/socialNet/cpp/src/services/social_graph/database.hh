#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <utils/mysql/client.hh>
#include <vector>


namespace socialNet::social_graph {

  struct Sub {
    uint32_t userId;
    uint32_t toWhom;
  };

  class SocialGraphDatabase {
  private:

    // The client connection to the DB
    std::shared_ptr <socialNet::utils::MysqlClient> _client;

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    SocialGraphDatabase ();

    /**
     * Configure the database
     */
    void configure (const rd_utils::utils::config::ConfigNode & conf);

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
    std::shared_ptr <rd_utils::memory::cache::collection::CacheArrayList<uint32_t> > findSubscriptions (uint32_t userId, uint32_t page, uint32_t nbPerPage);

    std::shared_ptr <utils::MysqlClient::Statement> prepareFindSubscriptions (uint32_t * resId, uint32_t uid, int32_t page, uint32_t nb);
    std::shared_ptr <utils::MysqlClient::Statement> prepareFindFollowers (uint32_t * resId, uint32_t uid, int32_t page, uint32_t nb);

    /**
     * Find the followers of a user
     * @params:
     *    - userId: the user being followed
     *    - page: the page index
     *    - nbPerPage: the number of subs per page
     */
    std::shared_ptr <rd_utils::memory::cache::collection::CacheArrayList<uint32_t> > findFollowers (uint32_t userId, uint32_t page, uint32_t nbPerPage);

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

  private:

    /**
     * Create the post and tags tables
     */
    void createTables ();

  };

}
