#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <utils/mysql/client.hh>
#include <vector>


namespace socialNet::post {

  struct Post {
    uint32_t userId;
    char userLogin [16];
    char text [512];
    uint32_t tags [16];
    uint8_t nbTags;
  };

  class PostDatabase {
  private:

    // The client connection to the DB
    std::shared_ptr <socialNet::utils::MysqlClient> _client;

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    PostDatabase ();

    /**
     * Configure the database
     */
    void configure (const rd_utils::utils::config::ConfigNode & configPath);

    /**
     * Insert a new post in the DB
     * @params:
     *    - post: the post to insert
     * @returns: the uniq id of the post
     */
    uint32_t insertPost (Post & p);

    /**
     * Find a post from its uniq Id
     * @params:
     *    - postId: the id of the post
     * @returns:
     *    - p: the found post (if returning true, unchanged otherwise)
     *    - return: false iif not found
     */
    bool findPost (uint32_t postId, Post & p);

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

  };

}

std::ostream& operator<< (std::ostream & s, const socialNet::post::Post & p);
