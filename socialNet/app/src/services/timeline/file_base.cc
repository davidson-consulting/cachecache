#include "file_base.hh"
#include <stdio.h>


namespace socialNet::timeline {

    uint32_t TimelineFileBase::BUFFER_MAX = 8192;

    TimelineFileBase::TimelineFileBase () {}

    void TimelineFileBase::configure (const std::string &ch, const rd_utils::utils::config::ConfigNode & conf) {
        if (conf.contains ("cache") && ch != "") {
            auto cacheAddr = conf["cache"][ch].getOr ("addr", "localhost");
            auto cachePort = conf["cache"][ch].getOr ("port", 6650);
            this-> _cache = std::make_shared <socialNet::utils::CacheClient> (cacheAddr, cachePort);
        }

        rd_utils::utils::create_directory ("./.home/", true);
        rd_utils::utils::create_directory ("./.post/", true);
    }

    void TimelineFileBase::dispose () {}


    void TimelineFileBase::insertHome (uint32_t uid, uint32_t pid) {
        WITH_LOCK (this-> _m) {
            this-> _bufferHome [uid].push_back (pid);
            this-> _bufferSize += 1;
        }

        if (this-> _bufferSize > BUFFER_MAX) this-> commit ();
    }

    void TimelineFileBase::insertPost (uint32_t uid, uint32_t pid) {
        WITH_LOCK (this-> _m) {
            this-> _bufferPost [uid].push_back (pid);
            this-> _bufferSize += 1;
        }

        if (this-> _bufferSize > BUFFER_MAX) this-> commit ();
    }

    void TimelineFileBase::commit () {
        WITH_LOCK (this-> _m) {
            for (auto & it : this-> _bufferPost) {
                FILE * f = ::fopen ((std::string (".post/_") + std::to_string (it.first)).c_str (), "a");
                if (f != nullptr) {
                    uint32_t fd = fileno (f);
                    if (fd == 0) fclose (f);
                    else {
                        lockf (fd, F_LOCK, 0);
                        fseek (f, 0, SEEK_END);
                        fwrite (it.second.data (), sizeof (uint32_t), it.second.size (), f);
                        lockf (fd, F_ULOCK, 0);
                        fclose (f);
                    }
                }
            }

            for (auto & it : this-> _bufferHome) {
                FILE * f = ::fopen ((std::string (".home/_") + std::to_string (it.first)).c_str (), "a");
                if (f != nullptr) {
                    uint32_t fd = fileno (f);
                    if (fd == 0) fclose (f);
                    else {
                        lockf (fd, F_LOCK, 0);
                        fseek (f, 0, SEEK_END);
                        fwrite (it.second.data (), sizeof (uint32_t), it.second.size (), f);
                        lockf (fd, F_ULOCK, 0);
                        fclose (f);
                    }
                }
            }

            this-> _bufferHome.clear ();
            this-> _bufferPost.clear ();
            this-> _bufferSize = 0;
        }
    }

    std::vector <uint32_t> TimelineFileBase::findHomeCacheable (uint32_t uid, uint32_t page, uint32_t nb) {
        std::vector <uint32_t> result;
        result.reserve (nb);

        if (this-> findHomeInCache (result, uid, page, nb)) return result;

        WITH_LOCK (this-> _m) {
            FILE * f = fopen ((std::string (".home/_") + std::to_string (uid)).c_str (), "r");

            if (f == nullptr) {
                return {};
            }

            uint32_t fd = fileno (f);
            if (fd == 0) {
                fclose (f);
                return {};
            }

            lockf (fd, F_LOCK, 0);
            fseek(f, 0, SEEK_END);
            uint32_t nbAll = ftell (f) / sizeof (uint32_t);
            if (page < nbAll) {
                fseek (f, page * sizeof (uint32_t), SEEK_SET);
                auto toRead = std::min (nbAll - page, nb);
                result.resize (toRead);

                fread (result.data (), sizeof (uint32_t), toRead, f);
            }

            lockf (fd, F_ULOCK, 0);
            fclose (f);
        }

        std::reverse(result.begin(), result.end());

        this-> insertHomeInCache (result, uid, page, nb);
        return result;
    }

    std::vector <uint32_t> TimelineFileBase::findPostCacheable (uint32_t uid, uint32_t page, uint32_t nb) {
        std::vector <uint32_t> result;
        result.reserve (nb);

        if (this-> findPostInCache (result, uid, page, nb)) return result;

        WITH_LOCK (this-> _m) {
            FILE * f = fopen ((std::string (".post/_") + std::to_string (uid)).c_str (), "r");
            if (f == nullptr) {
                return {};
            }

            uint32_t fd = fileno (f);
            if (fd == 0) {
                fclose (f);
                return {};
            }

            lockf (fd, F_LOCK, 0);
            fseek(f, 0, SEEK_END);
            uint32_t nbAll = ftell (f) / sizeof (uint32_t);
            if (page < nbAll) {
                fseek (f, page * sizeof (uint32_t), SEEK_SET);
                auto toRead = std::min (nbAll - page, nb);
                result.resize (toRead);

                fread (result.data (), sizeof (uint32_t), toRead, f);
            }

            lockf (fd, F_ULOCK, 0);
            fclose (f);
        }

        std::reverse (result.begin(), result.end());

        this-> insertPostInCache (result, uid, page, nb);
        return result;
    }



    uint32_t TimelineFileBase::countPosts (uint32_t uid) {
        WITH_LOCK (this-> _m) {
            FILE * f = fopen ((std::string (".post/_") + std::to_string (uid)).c_str (), "r");
            if (f == nullptr) return 0;

            uint32_t fd = fileno (f);
            if (fd == 0) {
                fclose (f);
                return 0;
            }

            lockf (fd, F_LOCK, 0);


            fseek(f, 0, SEEK_END);
            uint64_t sz = ftell(f);

            lockf (fd, F_ULOCK, 0);
            fclose (f);

            return sz / sizeof (uint32_t);
        }
    }

    uint32_t TimelineFileBase::countHome (uint32_t uid) {
        WITH_LOCK (this-> _m) {
            FILE * f = fopen ((std::string (".home/_") + std::to_string (uid)).c_str (), "r");
            if (f == nullptr) return 0;

            uint32_t fd = fileno (f);
            if (fd == 0) {
                fclose (f);
                return 0;
            }

            lockf (fd, F_LOCK, 0);

            fseek(f, 0, SEEK_END);
            uint64_t sz = ftell(f);

            lockf (fd, F_ULOCK, 0);

            fclose (f);
            return sz / sizeof (uint32_t);
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          CACHE          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */


    bool TimelineFileBase::findPostInCache (std::vector <uint32_t> & res, uint32_t uid, int32_t page, int32_t nb) {
        if (this-> _cache == nullptr) return false;
        if (this-> _cache-> get ("post_t:" + std::to_string (uid) + ":" + std::to_string (page) + ":" + std::to_string (nb), res)) {
            return true;
        }

        return false;
    }

    void TimelineFileBase::insertPostInCache (const std::vector <uint32_t> & res, uint32_t uid, int32_t page, int32_t nb) {
        if (this-> _cache == nullptr || res.size () <= 1) return;

        auto log =  "post_t:" + std::to_string (uid) + ":" + std::to_string (page) + ":" + std::to_string (nb);
        this-> _cache-> set (log, reinterpret_cast <const uint8_t*> (res.data ()), res.size () * sizeof (uint32_t));
    }

    bool TimelineFileBase::findHomeInCache (std::vector <uint32_t> & res, uint32_t uid, int32_t page, int32_t nb) {
        if (this-> _cache == nullptr) return false;
        if (this-> _cache-> get ("home_t:" + std::to_string (uid) + ":" + std::to_string (page) + ":" + std::to_string (nb), res)) {
            return true;
        }

        return false;
    }

    void TimelineFileBase::insertHomeInCache (const std::vector <uint32_t> & res, uint32_t uid, int32_t page, int32_t nb) {
        if (this-> _cache == nullptr || res.size () <= 1) return;

        auto log =  "home_t:" + std::to_string (uid) + ":" + std::to_string (page) + ":" + std::to_string (nb);
        this-> _cache-> set (log, reinterpret_cast <const uint8_t*> (res.data ()), res.size () * sizeof (uint32_t));
    }

    bool TimelineFileBase::hasCache () const {
        return this-> _cache != nullptr;
    }

}
