#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <utils/mysql/client.hh>
#include <vector>


namespace socialNet::timeline {

  class TimelineDatabase {
  private:

    // The client connection to the DB
    std::shared_ptr <socialNet::utils::MysqlClient> _client;

  public:

    /**
     * @params:
     *    - configPath: path to the configuration file
     */
    TimelineDatabase ();

    /**
     * Configure the database
     */
    void configure (const rd_utils::utils::config::ConfigNode & configPath);

    void insertOneHome (uint32_t uid, uint32_t pid);
    void insertOnePost (uint32_t uid, uint32_t pid);

    void executeBuffered (uint32_t * uid, uint32_t* pid, uint32_t nb);


    std::shared_ptr <utils::MysqlClient::Statement> prepareInsertHomeTimeline (uint32_t * uid, uint32_t * pid);

    std::shared_ptr <utils::MysqlClient::Statement> prepareFindHome (uint32_t * pid, uint32_t * uid, int32_t * page, int32_t * nb);
    std::shared_ptr <utils::MysqlClient::Statement> prepareFindPosts (uint32_t * pid, uint32_t * uid, int32_t * page, int32_t * nb);

    uint32_t countPosts (uint32_t id);
    uint32_t countHome (uint32_t id);

  private:

    /**
     * Create the post and tags tables
     */
    void createTables ();

  };

}
