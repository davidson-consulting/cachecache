#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <utils/mysql/client.hh>
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

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    UserDatabase ();

    /**
     * Configure the database
     */
    void configure (const rd_utils::utils::config::ConfigNode & configPath);

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

  };

}
