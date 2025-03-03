#pragma once

#include <string>
#include <rd_utils/_.hh>

#include "base.hh"

namespace socialNet::timeline {

        class FileTimelineDatabase : public TimelineDatabase {

                // The insertion buffer
                std::map <uint32_t, std::vector <uint32_t> > _bufferHome;
                std::map <uint32_t, std::vector <uint32_t> > _bufferPost;

                uint32_t _bufferSize;

                static uint32_t BUFFER_MAX;// = 8192;
                rd_utils::concurrency::mutex _m;
                std::string _homeDir;
                std::string _postDir;

        public:

                FileTimelineDatabase ();

                void configure (const std::string & dbname, const std::string & ch, const rd_utils::utils::config::ConfigNode & configPath);

                void insertHome (uint32_t uid, uint32_t pid) override;
                void insertPost (uint32_t uid, uint32_t pid) override;

                std::vector <uint32_t> findHome (uint32_t uid, uint32_t page, uint32_t nb) override;
                std::vector <uint32_t> findPost (uint32_t uid, uint32_t page, uint32_t nb) override;

                void commit () override;

                uint32_t countPosts (uint32_t id) override;
                uint32_t countHome (uint32_t id) override;

                void dispose () override;

        };



    
}
