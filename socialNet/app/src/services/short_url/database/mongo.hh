#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

#include <utils/mongo/client.hh>
#include "base.hh"

namespace socialNet::short_url {

  class MongoShortUrlDatabase : public ShortUrlDatabase {
  private:

    // The client connection to the DB
    std::shared_ptr <socialNet::utils::MongoPool> _client;

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    MongoShortUrlDatabase ();

    /**
     * Configure the database
     */
    void configure (const std::string & db, const std::string &, const rd_utils::utils::config::ConfigNode & configPath) override;

    /**
     * Insert a new post in the DB
     * @params:
     *    - post: the post to insert
     * @returns: the uniq id of the post
     */
    void insertUrl (ShortUrl & p) override;

    /**
     * Find a post from its uniq Id
     * @params:
     *    - url.hash: the hash of the url
     *    - url.ln: the (truncated) long url
     * @returns:
     *    - url.sh: the found url (if returning true, unchanged otherwise)
     *    - return: false iif not found
     */
    bool findUrl (ShortUrl & url) override;

        /**
     * Find a post from its uniq Id
     * @params:
     *    - url.hash: the hash of the url
     *    - url.sh: the short url
     * @returns:
     *    - url.ln: the found url (if returning true, unchanged otherwise)
     *    - return: false iif not found
     */
    bool findUrlFromShort (ShortUrl & url) override;

    /**
     * Dispose the sql connection
     */
    void dispose () override;

  private:

    /**
     * Create the post and tags tables
     */
    void createTables ();

  };

}
