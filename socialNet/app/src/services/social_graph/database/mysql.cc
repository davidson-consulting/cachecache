#include "mysql.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::social_graph {

  MysqlSocialGraphDatabase::MysqlSocialGraphDatabase () :
    _client (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MysqlSocialGraphDatabase::configure (const std::string & db, const std::string & ch, const config::ConfigNode & conf) {
    auto dbName = conf ["db"][db].getOr ("name", "socialNet");
    auto dbAddr = conf ["db"][db].getOr ("addr", "localhost");
    auto dbUser = conf ["db"][db].getOr ("user", "root");
    auto dbPass = conf ["db"][db].getOr ("pass", "root");
    auto dbPort = conf ["db"][db].getOr ("port", 0);

    this-> _client = std::make_shared <socialNet::utils::MysqlClient> (dbAddr, dbUser, dbPass, dbName, dbPort);
    this-> _client-> connect ();
    this-> createTables ();
    this-> configureCache (ch, conf);
  }

  void MysqlSocialGraphDatabase::createTables () {
    auto reqSocialGraph = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS subs (\n"
                                                    "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                                    "user_id INT NOT NULL,\n"
                                                    "to_whom INT NOT NULL\n)"
                                                    );
    reqSocialGraph-> finalize ();
    reqSocialGraph-> execute ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          INSERT/FIND/RM          =================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MysqlSocialGraphDatabase::insertSub (Sub & p) {
    auto req = this-> _client-> prepare ("INSERT INTO subs (user_id, to_whom) values (?, ?)");
    req-> setParam (0, &p.userId);
    req-> setParam (1, &p.toWhom);

    req-> finalize ();
    req-> execute ();
  }

  bool MysqlSocialGraphDatabase::findSub (Sub s) {
    uint32_t id;
    auto req = this-> _client-> prepare ("SELECT to_whom from subs where user_id = ? and to_whom = ?");
    req-> setParam (0, &s.userId);
    req-> setParam (1, &s.toWhom);
    req-> setResult (0, &id);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      return true;
    }

    return false;
  }

  void MysqlSocialGraphDatabase::removeSub (Sub sub) {
    auto req = this-> _client-> prepare ("DELETE from subs where user_id=? and to_whom=?");
    req-> setParam (0, &sub.userId);
    req-> setParam (1, &sub.toWhom);

    req-> finalize ();
    req-> execute ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          FIND LIST          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::vector <uint32_t> MysqlSocialGraphDatabase::findFollowers (uint32_t uid, uint32_t page, uint32_t nb) {
    std::vector <uint32_t> result;
    result.reserve (nb);
    if (this-> findFollowersInCache (result, uid, page, nb)) return result;

    uint32_t pid;
    uint32_t offset = page * nb;
    auto statement = this-> prepareFindFollowers (&pid, &uid, &offset, &nb);
    statement-> execute ();
    while (statement-> next ()) {
      result.push_back (pid);
    }

    if (result.size () != 0) {
      this-> insertFollowersInCache (result, uid, page, nb);
    }

    return result;
  }

  std::vector <uint32_t> MysqlSocialGraphDatabase::findSubs (uint32_t uid, uint32_t page, uint32_t nb) {
    std::vector <uint32_t> result;
    result.reserve (nb);

    if (this-> findSubsInCache (result, uid, page, nb)) return result;

    uint32_t pid;
    uint32_t offset = page * nb;
    auto statement = this-> prepareFindSubscriptions (&pid, &uid, &offset, &nb);
    statement-> execute ();
    while (statement-> next ()) {
      result.push_back (pid);
    }

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


  uint32_t MysqlSocialGraphDatabase::countNbSubs (uint32_t id) {
    auto req = this-> _client-> prepare ("SELECT COUNT(user_id) from subs where user_id=?");
    req-> setParam (0, &id);

    uint32_t nb = 0;
    req-> setResult (0, &nb);
    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      return nb;
    }

    return 0;
  }

  uint32_t MysqlSocialGraphDatabase::countNbFollowers (uint32_t id) {
    auto req = this-> _client-> prepare ("SELECT COUNT(user_id) from subs where to_whom=?");
    req-> setParam (0, &id);

    uint32_t nb = 0;
    req-> setResult (0, &nb);
    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      return nb;
    }

    return 0;
  }

  void MysqlSocialGraphDatabase::dispose () {
    this-> _client-> dispose ();
    this-> _client.reset ();
    SocialGraphDatabase::dispose ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          PRIVATE          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <utils::MysqlClient::Statement> MysqlSocialGraphDatabase::prepareFindSubscriptions (uint32_t * rid, uint32_t * uid, uint32_t * page, uint32_t * nb) {
    auto req = this-> _client-> prepare ("SELECT to_whom from subs where user_id = ? order by id DESC limit ? offset ?");
    req-> setParam (0, uid);
    req-> setParam (1, nb);
    req-> setParam (2, page);

    req-> setResult (0, rid);
    req-> finalize ();
    return req;
  }

  std::shared_ptr <utils::MysqlClient::Statement> MysqlSocialGraphDatabase::prepareFindFollowers (uint32_t * rid, uint32_t * uid, uint32_t * page, uint32_t * nb) {
    auto req = this-> _client-> prepare ("SELECT user_id from subs where to_whom = ? order by id DESC limit ? offset ?");
    req-> setParam (0, uid);
    req-> setParam (1, nb);
    req-> setParam (2, page);

    req-> setResult (0, rid);
    req-> finalize ();
    return req;
  }

}
