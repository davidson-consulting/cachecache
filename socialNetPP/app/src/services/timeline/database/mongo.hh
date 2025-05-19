#pragma once

#include <string>
#include <rd_utils/_.hh>
#include <utils/mongo/client.hh>

#include "base.hh"

namespace socialNet::timeline {

    class MongoTimelineDatabase : public TimelineDatabase {


        struct Time {
            uint32_t uid;
            uint32_t pid;
        };


        // The insertion buffer
        std::vector <Time> _bufferHome;
        std::vector <Time> _bufferPost;
        uint32_t _bufferSize;

        std::shared_ptr <socialNet::utils::MongoPool> _client;
        static uint32_t BUFFER_MAX;// = 8192;
        rd_utils::concurrency::mutex _m;

    public:

        MongoTimelineDatabase ();

        void configure (const std::string & dbname, const std::string & ch, const rd_utils::utils::config::ConfigNode & configPath);

        void insertHome (uint32_t uid, uint32_t pid) override;
        void insertPost (uint32_t uid, uint32_t pid) override;

        std::vector <uint32_t> findHome (uint32_t uid, uint32_t page, uint32_t nb) override;
        std::vector <uint32_t> findPost (uint32_t uid, uint32_t page, uint32_t nb) override;

        void commit () override;

        uint32_t countPosts (uint32_t id) override;
        uint32_t countHome (uint32_t id) override;

        void dispose () override;

    private:

        void insertInCollection (mongoc_collection_t * coll, const std::vector <Time> & elements);
        void performInsert (mongoc_collection_t * coll, const Time * datas, bson_t ** querys, uint32_t nb);



    };

}
