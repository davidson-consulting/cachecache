#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <utils/mongo/client.hh>
#include <vector>

#include "base.hh"

namespace socialNet::user {

  class MongoUserDatabase : public UserDatabase {
  private:

    // The client connection to the DB
    std::shared_ptr <socialNet::utils::MongoPool> _client;

    uint32_t _machineId;
    uint32_t _counter;

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    MongoUserDatabase (uint32_t uniqId);

    /**
     * Configure the database
     */
    void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & cfg) override;

    /**
     * Insert a new user in the db
     */
    uint32_t insertUser (User & p) override;

    /**
     * Find a user by its login
     */
    bool findByLogin (const std::string & login, User & u) override;

    /**
     * Find a user by its id
     */
    bool findById (uint32_t & id, User & u) override;


    void dispose () override;

  private:

    /**
     * Create the post and tags tables
     */
    void createTables ();

  };

}
