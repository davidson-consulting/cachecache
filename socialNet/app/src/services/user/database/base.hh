#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <utils/cache/client.hh>
#include <vector>

#define LOGIN_LEN 16
#define PASSWORD_LEN 64

namespace socialNet::user {

  struct User {
    uint32_t id;
    char login [LOGIN_LEN];
    char password [PASSWORD_LEN];
  };

  class UserDatabase {
  private:

    // The connection to the cache
    std::shared_ptr <socialNet::utils::CacheClient> _cache;

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    UserDatabase ();

    /**
     * Configure the database
     */
    virtual void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & cfg) = 0;

    /**
     * Insert a new user in the db
     */
    virtual uint32_t insertUser (User & p) = 0;

    /**
     * Find a user by its login
     */
    virtual bool findByLogin (const std::string & login, User & u) = 0;

    /**
     * Find a user by its id
     */
    virtual bool findById (uint32_t & id, User & u) = 0;


    virtual void dispose ();

  protected:

    void configureCache (const std::string & ch, const rd_utils::utils::config::ConfigNode & conf);

    /**
     * Find the user in the cache
     */
    bool findByLoginInCache (const std::string & login, User & u);

    /**
     * Insert a user in the cache
     */
    void insertByLoginInCache (User & u);

    /**
     * Find the user in the cache
     */
    bool findByIdInCache (uint32_t id, User & u);

    /**
     * Insert a user in the cache
     */
    void insertByIdInCache (User & u);

  };

}
