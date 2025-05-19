#include "base.hh"


namespace socialNet::timeline {

    TimelineDatabase::TimelineDatabase () {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          CONFIGURATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */


    void TimelineDatabase::configureCache (const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) {
        if (conf.contains ("cache") && ch != "") {
            auto cacheAddr = conf["cache"][ch].getOr ("addr", "localhost");
            auto cachePort = conf["cache"][ch].getOr ("port", 6650);
            this-> _cache = std::make_shared <socialNet::utils::CacheClient> (cacheAddr, cachePort);
        }
    }

    void TimelineDatabase::dispose () {
        this-> _cache.reset ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          CACHE          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    bool TimelineDatabase::findPostInCache (std::vector <uint32_t> & res, uint32_t uid, int32_t page, int32_t nb) {
        if (this-> _cache == nullptr) return false;
        if (this-> _cache-> get ("post_t:" + std::to_string (uid) + ":" + std::to_string (page) + ":" + std::to_string (nb), res)) {
            return true;
        }

        return false;
    }

    void TimelineDatabase::insertPostInCache (const std::vector <uint32_t> & res, uint32_t uid, int32_t page, int32_t nb) {
        if (this-> _cache == nullptr || res.size () <= 1) return;

        auto log =  "post_t:" + std::to_string (uid) + ":" + std::to_string (page) + ":" + std::to_string (nb);
        this-> _cache-> set (log, reinterpret_cast <const uint8_t*> (res.data ()), res.size () * sizeof (uint32_t));
    }

    bool TimelineDatabase::findHomeInCache (std::vector <uint32_t> & res, uint32_t uid, int32_t page, int32_t nb) {
        if (this-> _cache == nullptr) return false;
        if (this-> _cache-> get ("home_t:" + std::to_string (uid) + ":" + std::to_string (page) + ":" + std::to_string (nb), res)) {
            return true;
        }

        return false;
    }

    void TimelineDatabase::insertHomeInCache (const std::vector <uint32_t> & res, uint32_t uid, int32_t page, int32_t nb) {
        if (this-> _cache == nullptr || res.size () <= 1) return;

        auto log =  "home_t:" + std::to_string (uid) + ":" + std::to_string (page) + ":" + std::to_string (nb);
        this-> _cache-> set (log, reinterpret_cast <const uint8_t*> (res.data ()), res.size () * sizeof (uint32_t));
    }


}
