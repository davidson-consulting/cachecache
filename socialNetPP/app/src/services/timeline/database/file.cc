#include "file.hh"
#include <stdio.h>
#include <rd_utils/utils/files.hh>

using namespace rd_utils;

namespace socialNet::timeline {

    uint32_t FileTimelineDatabase::BUFFER_MAX = 8192;

    FileTimelineDatabase::FileTimelineDatabase () {}

    void FileTimelineDatabase::configure (const std::string & db, const std::string &ch, const rd_utils::utils::config::ConfigNode & conf) {
        this-> configureCache (ch, conf);
        CONF_LET (cwd, conf ["db"][db]["directory"].getStr (), std::string ("."));
        this-> _postDir = rd_utils::utils::join_path (cwd, ".post/");
        this-> _homeDir = rd_utils::utils::join_path (cwd, ".home/");

        rd_utils::utils::create_directory (this-> _postDir, true);
        rd_utils::utils::create_directory (this-> _homeDir, true);
    }

    void FileTimelineDatabase::dispose () {
        TimelineDatabase::dispose ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          INSERTION          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void FileTimelineDatabase::insertHome (uint32_t uid, uint32_t pid) {
        WITH_LOCK (this-> _m) {
            this-> _bufferHome [uid].push_back (pid);
            this-> _bufferSize += 1;
        }

        if (this-> _bufferSize > BUFFER_MAX) this-> commit ();
    }

    void FileTimelineDatabase::insertPost (uint32_t uid, uint32_t pid) {
        WITH_LOCK (this-> _m) {
            this-> _bufferPost [uid].push_back (pid);
            this-> _bufferSize += 1;
        }

        if (this-> _bufferSize > BUFFER_MAX) this-> commit ();
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          FINDING          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::vector <uint32_t> FileTimelineDatabase::findHome (uint32_t uid, uint32_t page, uint32_t nb) {
        std::vector <uint32_t> result;
        result.reserve (nb);
        if (nb == 0) return result;

        if (this-> findHomeInCache (result, uid, page, nb)) return result;

        WITH_LOCK (this-> _m) {
            FILE * f = fopen (rd_utils::utils::join_path (this-> _homeDir, "_" + std::to_string (uid)).c_str (), "r");

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
            uint32_t nbPages = nbAll / nb;

            if (nbPages == 0) { // readAll
                if (page == 0) {
                    fseek (f, 0, SEEK_SET);
                    result.resize (nbAll);
                    fread (result.data (), sizeof (uint32_t), nbAll, f);
                }
            }

            else if (nbAll >= (page + 1) * nb) {
                uint32_t offset = nbAll - ((page + 1) * nb);

                fseek (f, offset * sizeof (uint32_t), SEEK_SET);
                auto toRead = std::min (nbAll - offset, nb);

                result.resize (toRead);

                fread (result.data (), sizeof (uint32_t), toRead, f);
            } else if (nbAll >= (page * nb)) {
                fseek (f, 0, SEEK_SET);
                auto toRead = ((page + 1) * nb) - nbAll;
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

    std::vector <uint32_t> FileTimelineDatabase::findPost (uint32_t uid, uint32_t page, uint32_t nb) {
        std::vector <uint32_t> result;
        result.reserve (nb);

        if (this-> findPostInCache (result, uid, page, nb)) return result;

        WITH_LOCK (this-> _m) {
            FILE * f = fopen (rd_utils::utils::join_path (this-> _postDir, "_" + std::to_string (uid)).c_str (), "r");
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
            uint32_t nbPages = nbAll / nb;

            if (nbPages == 0) { // readAll
                if (page == 0) {
                    fseek (f, 0, SEEK_SET);
                    result.resize (nbAll);
                    fread (result.data (), sizeof (uint32_t), nbAll, f);
                }
            }

            else if (nbAll >= (page + 1) * nb) {
                uint32_t offset = nbAll - ((page + 1) * nb);

                fseek (f, offset * sizeof (uint32_t), SEEK_SET);
                auto toRead = std::min (nbAll - offset, nb);

                result.resize (toRead);

                fread (result.data (), sizeof (uint32_t), toRead, f);
            } else if (nbAll >= (page * nb)) {
                fseek (f, 0, SEEK_SET);
                auto toRead = ((page + 1) * nb) - nbAll;
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

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          COUNTING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint32_t FileTimelineDatabase::countPosts (uint32_t uid) {
        WITH_LOCK (this-> _m) {
            FILE * f = fopen (rd_utils::utils::join_path (this-> _postDir, "_" + std::to_string (uid)).c_str (), "r");
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

    uint32_t FileTimelineDatabase::countHome (uint32_t uid) {
        WITH_LOCK (this-> _m) {
            FILE * f = fopen (rd_utils::utils::join_path (this-> _homeDir, "_" + std::to_string (uid)).c_str (), "r");
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
     * =====================================          COMMIT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void FileTimelineDatabase::commit () {
        WITH_LOCK (this-> _m) {
            for (auto & it : this-> _bufferPost) {
                FILE * f = fopen (rd_utils::utils::join_path (this-> _postDir, "_" + std::to_string (it.first)).c_str (), "a");
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
                FILE * f = fopen (rd_utils::utils::join_path (this-> _homeDir, "_" + std::to_string (it.first)).c_str (), "a");
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


}
