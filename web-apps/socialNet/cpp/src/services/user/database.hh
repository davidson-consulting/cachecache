#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <utils/mysql/client.hh>
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

    // The client connection to the DB
    std::shared_ptr <socialNet::utils::MysqlClient> _client;

    // The connection to the cache
    std::shared_ptr <socialNet::utils::CacheClient> _cache;

    uint64_t _hit = 0;
    uint64_t _fail = 0;

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    UserDatabase ();

    /**
     * Configure the database
     */
    void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & cfg);

    /**
     * Insert a new user in the db
     */
    uint32_t insertUser (User & p);

    /**
     * Find a user by its login
     */
    bool findByLogin (const std::string & login, User & u);

    /**
     * Find a user by its id
     */
    bool findById (uint32_t & id, User & u);

  private:

    /**
     * Create the post and tags tables
     */
    void createTables ();

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
