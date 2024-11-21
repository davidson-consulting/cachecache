#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

#include <utils/mysql/client.hh>
#include <utils/cache/client.hh>


#define SHORT_LEN 16
#define LONG_LEN 255

namespace socialNet::short_url {

  struct ShortUrl {
    char sh[SHORT_LEN]; // short version
    char ln[LONG_LEN]; // start of the long version
  };

  class ShortUrlDatabase {
  private:

    // The client connection to the DB
    std::shared_ptr <socialNet::utils::MysqlClient> _client;

    std::shared_ptr <socialNet::utils::CacheClient> _cache;

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    ShortUrlDatabase ();

    /**
     * Configure the database
     */
    void configure (const std::string & db, const std::string &, const rd_utils::utils::config::ConfigNode & configPath);

    /**
     * Insert a new post in the DB
     * @params:
     *    - post: the post to insert
     * @returns: the uniq id of the post
     */
    uint32_t insertUrl (ShortUrl & p);

    /**
     * Find a post from its uniq Id
     * @params:
     *    - url.hash: the hash of the url
     *    - url.ln: the (truncated) long url
     * @returns:
     *    - url.sh: the found url (if returning true, unchanged otherwise)
     *    - return: false iif not found
     */
    bool findUrl (ShortUrl & url);

        /**
     * Find a post from its uniq Id
     * @params:
     *    - url.hash: the hash of the url
     *    - url.sh: the short url
     * @returns:
     *    - url.ln: the found url (if returning true, unchanged otherwise)
     *    - return: false iif not found
     */
    bool findUrlFromShort (ShortUrl & url);

    /**
     * Dispose the sql connection
     */
    void dispose ();

  private:

    /**
     * Create the post and tags tables
     */
    void createTables ();

  };

}

std::ostream& operator<< (std::ostream & s, socialNet::short_url::ShortUrl & p);
