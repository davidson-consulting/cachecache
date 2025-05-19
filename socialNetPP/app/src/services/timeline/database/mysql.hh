#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

#include <utils/mysql/client.hh>
#include "base.hh"

namespace socialNet::timeline {

    class MysqlTimelineDatabase : public TimelineDatabase {
    private:

        struct Time {
            uint32_t uid;
            uint32_t pid;
        };

        // The client connection to the DB
        std::shared_ptr <socialNet::utils::MysqlClient> _client;

        static uint32_t BUFFER_MAX;

        uint32_t _bufferSize;
        std::vector <Time> _bufferHome;
        std::vector <Time> _bufferPost;

        rd_utils::concurrency::mutex _m;

    public:

        /**
         * @params:
         *    - configPath: path to the configuration file
         */
        MysqlTimelineDatabase ();

        /**
         * Configure the database
         */
        void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & configPath) override;

        void insertHome (uint32_t uid, uint32_t pid) override;
        void insertPost (uint32_t uid, uint32_t pid) override;

        std::vector <uint32_t> findHome (uint32_t uid, uint32_t page, uint32_t nb) override;
        std::vector <uint32_t> findPost (uint32_t uid, uint32_t page, uint32_t nb) override;

        uint32_t countPosts (uint32_t id);
        uint32_t countHome (uint32_t id);

        void commit () override;
        void dispose () override;

    private:

        /**
         * Create the post and tags tables
         */
        void createTables ();

        std::shared_ptr <utils::MysqlClient::Statement> prepareFindHome (uint32_t * pid, uint32_t * uid, uint32_t * page, uint32_t * nb);
        std::shared_ptr <utils::MysqlClient::Statement> prepareFindPost (uint32_t * pid, uint32_t * uid, uint32_t * page, uint32_t * nb);
    };

}
