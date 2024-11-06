#include "database.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::timeline {

  TimelineDatabase::TimelineDatabase () :
    _client (nullptr)
  {}

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          CONFIGURATION          ==================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void TimelineDatabase::configure (const std::string & db, const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) {
    auto dbName = conf ["db"][db].getOr ("name", "socialNet");
    auto dbAddr = conf ["db"][db].getOr ("addr", "localhost");
    auto dbUser = conf ["db"][db].getOr ("user", "root");
    auto dbPass = conf ["db"][db].getOr ("pass", "root");

    this-> _client = std::make_shared <socialNet::utils::MysqlClient> (dbAddr, dbUser, dbPass, dbName);
    this-> _client-> connect ();
    this-> createTables ();

    if (conf.contains ("cache") && ch != "") {
      auto cacheAddr = conf["cache"][ch].getOr ("addr", "localhost");
      auto cachePort = conf["cache"][ch].getOr ("port", 6650);
      this-> _cache = std::make_shared <socialNet::utils::CacheClient> (cacheAddr, cachePort);
    }
  }

  void TimelineDatabase::createTables () {
    auto reqTimeline = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS home_timeline (\n"
                                                 "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                                 "user_id INT NOT NULL,\n"
                                                 "post_id INT NOT NULL\n)"
                                                 );
    reqTimeline-> finalize ();
    reqTimeline-> execute ();

    auto reqPostTimeline = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS post_timeline (\n"
                                                     "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                                     "user_id INT NOT NULL,\n"
                                                     "post_id INT NOT NULL\n)"
                                                     );
    reqPostTimeline-> finalize ();
    reqPostTimeline-> execute ();
  }

  void TimelineDatabase::commit () {
    this-> _client-> commit ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          INSERT/FINDS          ==================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void TimelineDatabase::insertOneHome (uint32_t uid, uint32_t pid) {
    auto req = this-> prepareInsertHomeTimeline (&uid, &pid);
    req-> execute ();
  }

  void TimelineDatabase::insertOnePost (uint32_t uid, uint32_t pid) {
    auto req = this-> _client-> prepare ("INSERT INTO post_timeline (user_id, post_id) values (?, ?)");
    req-> setParam (0, &uid);
    req-> setParam (1, &pid);
    req-> finalize ();

    req-> execute ();
  }

  std::shared_ptr <utils::MysqlClient::Statement> TimelineDatabase::prepareInsertHomeTimeline (uint32_t * uid, uint32_t * pid) {
    auto req = this-> _client-> prepare ("INSERT INTO home_timeline (user_id, post_id) values (?, ?)");
    req-> setParam (0, uid);
    req-> setParam (1, pid);
    req-> finalize ();

    return req;
  }

  std::shared_ptr <utils::MysqlClient::Statement> TimelineDatabase::prepareBuffered (uint32_t nb) {
    std::stringstream ss;
    ss << "INSERT INTO home_timeline (user_id, post_id) values ";
    for (uint32_t i = 0 ; i < nb; i++) {
      if (i != 0) ss << ", ";
      ss << "(?, ?)";
    }

    auto req = this-> _client-> prepare (ss.str ());
    return req;
  }

  void TimelineDatabase::executeBuffered (std::shared_ptr <utils::MysqlClient::Statement> req, uint32_t* uids, uint32_t* pids, uint32_t nb) {
    for (uint32_t i = 0, j = 0 ; i < nb ; j += 2, i++) {
      req-> setParam (j, &uids [i]);
      req-> setParam (j + 1, &pids [i]);
    }

    req-> finalize ();
    req-> execute ();
  }

  std::shared_ptr <utils::MysqlClient::Statement> TimelineDatabase::prepareFindHome (uint32_t * pid, uint32_t * uid, int32_t * page, int32_t * nb) {
    if (*page >= 0) {
      auto req = this-> _client-> prepare ("SELECT post_id from home_timeline where user_id = ? order by id DESC limit ? offset ?");
      req-> setParam (0, uid);
      req-> setParam (1, nb);
      req-> setParam (2, page);
      req-> setResult (0, pid);
      req-> finalize ();
      
      return req;
    } else {
      auto req = this-> _client-> prepare ("SELECT post_id from home_timeline where user_id = ? order by id DESC");
      req-> setParam (0, uid);
      req-> setResult (0, pid);
      req-> finalize ();
      return req;
    }
  }

  std::shared_ptr <utils::MysqlClient::Statement> TimelineDatabase::prepareFindPosts (uint32_t * pid, uint32_t * uid, int32_t * page, int32_t * nb) {
    if (*page >= 0) {
      auto req = this-> _client-> prepare ("SELECT post_id from post_timeline where user_id = ? order by id DESC limit ? offset ?");

      req-> setParam (0, uid);
      req-> setParam (1, nb);
      req-> setParam (2, page);
      req-> setResult (0, pid);
      req-> finalize ();

      return req;
    } else {
      auto req = this-> _client-> prepare ("SELECT post_id from post_timeline where user_id = ? order by id DESC");
      req-> setParam (0, uid);
      req-> setResult (0, pid);
      req-> finalize ();
      return req;
    }
  }

  uint32_t TimelineDatabase::countPosts (uint32_t id) {
    auto req = this-> _client-> prepare ("SELECT COUNT(post_id) from post_timeline where user_id=?");
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

  uint32_t TimelineDatabase::countHome (uint32_t id) {
    auto req = this-> _client-> prepare ("SELECT COUNT(post_id) from home_timeline where user_id=?");
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

}
