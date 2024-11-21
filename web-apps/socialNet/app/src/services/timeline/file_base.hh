#pragma once

#include <string>
#include <rd_utils/_.hh>
#include <utils/cache/client.hh>

namespace socialNet::timeline {

    class TimelineFileBase {

        // The cache connection
        std::shared_ptr <socialNet::utils::CacheClient> _cache;

        // The insertion buffer
        std::map <uint32_t, std::vector <uint32_t> > _bufferHome;
        std::map <uint32_t, std::vector <uint32_t> > _bufferPost;

        uint32_t _bufferSize;

        static uint32_t BUFFER_MAX;// = 8192;

        rd_utils::concurrency::mutex _m;

    public:

        TimelineFileBase ();

        /**
         * Configure the file base
         */
        void configure (const std::string & ch, const rd_utils::utils::config::ConfigNode & configPath);

        /**
         * Insert a post in the insertion buffer
         * @warning: does not automatically commit
         */
        void insertHome (uint32_t uid, uint32_t pid);

        /**
         * Insert a post in the insertion buffer
         * @warning: does not automatically commit
         */
        void insertPost (uint32_t uid, uint32_t pid);

        /**
         * Write the insertion buffer to files
         */
        void commit ();

        std::vector <uint32_t> findHomeCacheable (uint32_t uid, uint32_t page, uint32_t nb);
        std::vector <uint32_t> findPostCacheable (uint32_t uid, uint32_t page, uint32_t nb);

        uint32_t countPosts (uint32_t id);
        uint32_t countHome (uint32_t id);

        bool hasCache () const;
        void dispose ();

    private:

        bool findHomeInCache (std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        void insertHomeInCache (const std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        bool findPostInCache (std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        void insertPostInCache (const std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);

    };



    
}
