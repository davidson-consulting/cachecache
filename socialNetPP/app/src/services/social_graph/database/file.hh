#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <rd_utils/memory/cache/_.hh>
#include <vector>

#include "base.hh"

namespace socialNet::social_graph {

        class FileSocialGraphDatabase : public SocialGraphDatabase {
        private:

                std::string _subscriptions;
                std::string _followers;

        public:

                FileSocialGraphDatabase ();

                void configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) override;

                void insertSub (Sub & p) override;
                bool findSub (Sub s) override;

                std::vector <uint32_t> findFollowers (uint32_t uid, uint32_t page, uint32_t nb) override;
                std::vector <uint32_t> findSubs (uint32_t uid, uint32_t page, uint32_t nb) override;

                uint32_t countNbSubs (uint32_t id) override;

                uint32_t countNbFollowers (uint32_t id) override;

                void removeSub (Sub sub) override;

                void dispose () override;

        private:

                void removeFollower (Sub sub);

        };

}
