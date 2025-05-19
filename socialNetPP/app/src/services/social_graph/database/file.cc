#include "file.hh"
#include <rd_utils/_.hh>
#include <nlohmann/json.hpp>
#include <stdio.h>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::social_graph {

  FileSocialGraphDatabase::FileSocialGraphDatabase () {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void FileSocialGraphDatabase::configure (const std::string & db, const std::string & ch, const config::ConfigNode & conf) {
    this-> configureCache (ch, conf);

    CONF_LET (cwd, conf ["db"][db]["directory"].getStr (), std::string ("."));
    this-> _subscriptions = rd_utils::utils::join_path (cwd, ".subs");
    this-> _followers = rd_utils::utils::join_path (cwd, ".folls");

    rd_utils::utils::create_directory (this-> _subscriptions, true);
    rd_utils::utils::create_directory (this-> _followers, true);
  }

  void FileSocialGraphDatabase::dispose () {
    SocialGraphDatabase::dispose ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERT/FIND/RM          =================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void FileSocialGraphDatabase::insertSub (Sub & s) {
    { // insert subscription
      std::string filename = rd_utils::utils::join_path (this-> _subscriptions, "_" + std::to_string (s.userId));
      FILE * f = fopen (filename.c_str (), "a");
      if (f != nullptr) {
        uint32_t fd = fileno (f);
        if (fd == 0) fclose (f);
        else {
          lockf (fd, F_LOCK, 0);
          fseek (f, 0, SEEK_END);
          fwrite (&s.toWhom, sizeof (uint32_t), 1, f);
          lockf (fd, F_ULOCK, 0);
          fclose (f);
        }
      }
    }

    { // insert follower
      std::cout << "Insert in followers : " << s.toWhom << " " << s.userId << std::endl;
      std::string filename = rd_utils::utils::join_path (this-> _followers, "_" + std::to_string (s.toWhom));
      FILE * f = fopen (filename.c_str (), "a");
      if (f != nullptr) {
        uint32_t fd = fileno (f);
        if (fd == 0) fclose (f);
        else {
          lockf (fd, F_LOCK, 0);
          fseek (f, 0, SEEK_END);
          fwrite (&s.userId, sizeof (uint32_t), 1, f);
          lockf (fd, F_ULOCK, 0);
          fclose (f);
        }
      }
    }
  }

  bool FileSocialGraphDatabase::findSub (Sub s) {
    bool found = false;

    std::string filename = rd_utils::utils::join_path (this-> _subscriptions, "_" + std::to_string (s.userId));
    FILE * f = fopen (filename.c_str (), "r");
    if (f != nullptr) {
      uint32_t fd = fileno (f);
      if (fd == 0) {
        fclose (f);
        return false;
      }

      lockf (fd, F_LOCK, 0);
      uint32_t buffer [255];

      for (uint32_t i = 0 ;; i++) {
        uint32_t nbRead = fread (buffer, sizeof (uint32_t), 255, f);
        uint32_t j = 0;
        for (j = 0 ; j < nbRead ; j++) {
          if (buffer [j] == s.toWhom) {
            found = true;
            break;
          }
        }

        if (nbRead != 255 || found) break;
      }

      lockf (fd, F_ULOCK, 0);
      fclose (f);
    }

    return found;
  }

  void FileSocialGraphDatabase::removeSub (Sub s) {
    std::string copyName = rd_utils::utils::join_path (this-> _subscriptions, "_" + std::to_string (s.userId) + "_copy");
    std::string oldName = rd_utils::utils::join_path (this-> _subscriptions, "_" + std::to_string (s.userId));
    FILE * f = fopen (oldName.c_str (), "r");
    if (f != nullptr) {
      FILE * out = fopen (copyName.c_str (), "a");
      if (out == nullptr) {
        fclose (f);
        return;
      }

      uint32_t fd = fileno (f);
      uint32_t fd2 = fileno (out);
      if (fd == 0 || fd2 == 0) {
        fclose (f);
        fclose (out);
      }

      lockf (fd, F_LOCK, 0);
      lockf (fd2, F_LOCK, 0);

      uint32_t buffer [255];
      uint32_t toWrite [255];
      bool found = false;

      for (uint32_t i = 0 ;; i++) {
        uint32_t nbRead = fread (buffer, sizeof (uint32_t), 255, f);
        uint32_t j = 0, z = 0;
        for (j = 0 ; j < nbRead ; j++) {
          if (buffer [j] != s.toWhom) {
            toWrite [z] = buffer [j];
            z ++;
          } else found = true;
        }

        fwrite (toWrite, sizeof (uint32_t), z, out);
        if (nbRead != 255) break;
      }

      lockf (fd, F_ULOCK, 0);
      lockf (fd2, F_ULOCK, 0);
      fclose (f);
      fclose (out);

      rename (copyName.c_str (), oldName.c_str ());
      if (found) removeFollower (s);
    }
  }

  void FileSocialGraphDatabase::removeFollower (Sub s) {
    std::string copyName = rd_utils::utils::join_path (this-> _followers, "_" + std::to_string (s.toWhom) + "_copy");
    std::string oldName = rd_utils::utils::join_path (this-> _followers, "_" + std::to_string (s.toWhom));
    FILE * f = fopen (oldName.c_str (), "r");
    if (f != nullptr) {
      FILE * out = fopen (copyName.c_str (), "a");
      if (out == nullptr) {
        fclose (f);
        return;
      }

      uint32_t fd = fileno (f);
      uint32_t fd2 = fileno (out);
      if (fd == 0 || fd2 == 0) {
        fclose (f);
        fclose (out);
      }

      lockf (fd, F_LOCK, 0);
      lockf (fd2, F_LOCK, 0);

      uint32_t buffer [255];
      uint32_t toWrite [255];

      for (uint32_t i = 0 ;; i++) {
        uint32_t nbRead = fread (buffer, sizeof (uint32_t), 255, f);
        uint32_t j = 0, z = 0;
        for (j = 0 ; j < nbRead ; j++) {
          if (buffer [j] != s.userId) {
            toWrite [z] = buffer [j];
            z ++;
          }
        }

        fwrite (toWrite, sizeof (uint32_t), z, out);
        if (nbRead != 255) break;
      }

      lockf (fd, F_ULOCK, 0);
      lockf (fd2, F_ULOCK, 0);
      fclose (f);
      fclose (out);

      rename (copyName.c_str (), oldName.c_str ());
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          FIND LIST          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::vector <uint32_t> FileSocialGraphDatabase::findFollowers (uint32_t uid, uint32_t page, uint32_t nb) {
    std::vector <uint32_t> result;
    result.reserve (nb);
    if (this-> findFollowersInCache (result, uid, page, nb)) return result;

    std::string filename = rd_utils::utils::join_path (this-> _followers, "_" + std::to_string (uid));
    FILE * f = fopen (filename.c_str (), "r");
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
    uint32_t offset = page * nb;
    if (offset < nbAll) {
      fseek (f, offset * sizeof (uint32_t), SEEK_SET);
      auto toRead = std::min (nbAll - offset, nb);
      result.resize (toRead);

      fread (result.data (), sizeof (uint32_t), toRead, f);
    }

    lockf (fd, F_ULOCK, 0);
    fclose (f);

    if (result.size () != 0) {
      this-> insertFollowersInCache (result, uid, page, nb);
    }

    return result;
  }

  std::vector <uint32_t> FileSocialGraphDatabase::findSubs (uint32_t uid, uint32_t page, uint32_t nb) {
    std::vector <uint32_t> result;
    result.reserve (nb);
    if (this-> findSubsInCache (result, uid, page, nb)) return result;

    std::string filename = rd_utils::utils::join_path (this-> _subscriptions, "_" + std::to_string (uid));
    FILE * f = fopen (filename.c_str (), "r");
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
    uint32_t offset = page * nb;
    if (offset < nbAll) {
      fseek (f, offset * sizeof (uint32_t), SEEK_SET);
      auto toRead = std::min (nbAll - offset, nb);
      result.resize (toRead);

      fread (result.data (), sizeof (uint32_t), toRead, f);
    }

    lockf (fd, F_ULOCK, 0);
    fclose (f);

    if (result.size () != 0) {
      this-> insertSubsInCache (result, uid, page, nb);
    }

    return result;
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =====================================          COUNT          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */


  uint32_t FileSocialGraphDatabase::countNbSubs (uint32_t uid) {
    std::string filename = rd_utils::utils::join_path (this-> _subscriptions, "_" + std::to_string (uid));
    FILE * f = fopen (filename.c_str (), "r");
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

  uint32_t FileSocialGraphDatabase::countNbFollowers (uint32_t uid) {
    std::string filename = rd_utils::utils::join_path (this-> _followers, "_" + std::to_string (uid));
    FILE * f = fopen (filename.c_str (), "r");
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
