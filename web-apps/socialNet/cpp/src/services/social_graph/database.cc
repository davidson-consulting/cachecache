#include "database.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::social_graph {

  SocialGraphDatabase::SocialGraphDatabase () :
    _client (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void SocialGraphDatabase::configure (const rd_utils::utils::config::ConfigNode & conf) {
    auto dbName = conf ["db"].getOr ("name", "socialNet");
    auto dbAddr = conf ["db"].getOr ("addr", "localhost");
    auto dbUser = conf ["db"].getOr ("user", "root");
    auto dbPass = conf ["db"].getOr ("pass", "root");

    this-> _client = std::make_shared <socialNet::utils::MysqlClient> (dbAddr, dbUser, dbPass, dbName);
    this-> _client-> connect ();
    this-> createTables ();
  }

  void SocialGraphDatabase::createTables () {
    auto reqSocialGraph = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS subs (\n"
                                                    "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                                    "user_id INT NOT NULL,\n"
                                                    "to_whom INT NOT NULL\n)"
                                                    );
    reqSocialGraph-> finalize ();
    reqSocialGraph-> execute ();
  }

  uint32_t SocialGraphDatabase::insertSub (Sub & p) {
    auto req = this-> _client-> prepare ("INSERT INTO subs (user_id, to_whom) values (?, ?)");
    req-> setParam (0, &p.userId);
    req-> setParam (1, &p.toWhom);

    req-> finalize ();
    req-> execute ();

    return req-> getGeneratedId ();
  }

  bool SocialGraphDatabase::findSub (Sub s) {
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

  std::shared_ptr <utils::MysqlClient::Statement> SocialGraphDatabase::prepareFindSubscriptions (uint32_t * rid, uint32_t * uid, int32_t * page, int32_t * nb) {
    if (*page >= 0) {
      auto req = this-> _client-> prepare ("SELECT to_whom from subs where user_id = ? order by id DESC limit ? offset ?");
      req-> setParam (0, uid);
      req-> setParam (1, nb);
      req-> setParam (2, page);

      req-> setResult (0, rid);
      req-> finalize ();
      return req;
    } else {
      auto req = this-> _client-> prepare ("SELECT to_whom from subs where user_id = ? order by id DESC");
      req-> setParam (0, uid);

      req-> setResult (0, rid);
      req-> finalize ();
      return req;
    }
  }

  std::shared_ptr <utils::MysqlClient::Statement> SocialGraphDatabase::prepareFindFollowers (uint32_t * rid, uint32_t * uid, int32_t * page, int32_t * nb) {
    if (*page >= 0) {
      auto req = this-> _client-> prepare ("SELECT user_id from subs where to_whom = ? order by id DESC limit ? offset ?");
      req-> setParam (0, uid);
      req-> setParam (1, nb);
      req-> setParam (2, page);

      req-> setResult (0, rid);
      req-> finalize ();
      return req;
    } else {
      auto req = this-> _client-> prepare ("SELECT user_id from subs where to_whom = ? order by id DESC");
      req-> setParam (0, uid);

      req-> setResult (0, rid);
      req-> finalize ();
      return req;
    }
  }

  uint32_t SocialGraphDatabase::countNbSubs (uint32_t id) {
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

  uint32_t SocialGraphDatabase::countNbFollowers (uint32_t id) {
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

  void SocialGraphDatabase::removeSub (Sub & sub) {
    auto req = this-> _client-> prepare ("DELETE from subs where user_id=? and to_whom=?");
    req-> setParam (0, &sub.userId);
    req-> setParam (1, &sub.toWhom);

    req-> finalize ();
    req-> execute ();
  }

}
