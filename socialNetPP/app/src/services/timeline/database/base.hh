#pragma once


#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

#include <utils/cache/client.hh>

namespace socialNet::timeline {

    class TimelineDatabase {
    private:

        std::shared_ptr <socialNet::utils::CacheClient> _cache;

    public:

        /**
         * @params:
         *    - configPath: path to the configuration file
         */
        TimelineDatabase ();

        /**
         * Configure the database
         */
        virtual void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & configPath) = 0;

        virtual void insertHome (uint32_t uid, uint32_t pid) = 0;

        virtual void insertPost (uint32_t uid, uint32_t pid) = 0;

        virtual void commit () = 0;

        virtual uint32_t countPosts (uint32_t id) = 0;

        virtual uint32_t countHome (uint32_t id) = 0;

        virtual std::vector <uint32_t> findHome (uint32_t uid, uint32_t page, uint32_t nb) = 0;

        virtual std::vector <uint32_t> findPost (uint32_t uid, uint32_t page, uint32_t nb) = 0;

        virtual void dispose ();

    protected:

        void configureCache (const std::string & ch, const rd_utils::utils::config::ConfigNode & conf);

        bool findHomeInCache (std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        void insertHomeInCache (const std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        bool findPostInCache (std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);
        void insertPostInCache (const std::vector <uint32_t> & result, uint32_t uid, int32_t page, int32_t nb);


    };

}
