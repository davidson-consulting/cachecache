#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

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
    virtual void configure (const std::string & db, const std::string &, const rd_utils::utils::config::ConfigNode & configPath) = 0;

    /**
     * Insert a new post in the DB
     * @params:
     *    - post: the post to insert
     * @returns: the uniq id of the post
     */
    virtual void insertUrl (ShortUrl & p) = 0;

    /**
     * Find a post from its uniq Id
     * @params:
     *    - url.hash: the hash of the url
     *    - url.ln: the (truncated) long url
     * @returns:
     *    - url.sh: the found url (if returning true, unchanged otherwise)
     *    - return: false iif not found
     */
    virtual bool findUrl (ShortUrl & url) = 0;

        /**
     * Find a post from its uniq Id
     * @params:
     *    - url.hash: the hash of the url
     *    - url.sh: the short url
     * @returns:
     *    - url.ln: the found url (if returning true, unchanged otherwise)
     *    - return: false iif not found
     */
    virtual bool findUrlFromShort (ShortUrl & url) = 0;

    /**
     * Dispose the sql connection
     */
    virtual void dispose ();

  protected:

    /**
     * Configure the cache of the db
     */
    void configureCache (const std::string & ch, const rd_utils::utils::config::ConfigNode & cfg);

    /**
     * Find a short url in the cache
     */
    bool findUrlFromShortInCache (ShortUrl & p);

    /**
     * Find a post in the cache
     */
    bool findUrlFromLongInCache (ShortUrl & p);

    /**
     * Insert a post in the cache
     */
    void insertUrlInCache (ShortUrl & p);

  };

}

std::ostream& operator<< (std::ostream & s, socialNet::short_url::ShortUrl & p);
