#include "mysql.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::user {

  MysqlUserDatabase::MysqlUserDatabase () :
    _client (nullptr)
  {}

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =================================          CONFIGURATION          ==================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MysqlUserDatabase::configure (const std::string & db, const std::string & ch, const config::ConfigNode & conf) {
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

  void MysqlUserDatabase::createTables () {
    auto reqUser = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS users (\n"
                                             "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                             "login VARCHAR(" + std::to_string (LOGIN_LEN) + ") NOT NULL,\n"
                                             "password VARCHAR(" + std::to_string (PASSWORD_LEN) + ") NOT NULL\n)"
                                             );
    reqUser-> finalize ();
    reqUser-> execute ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ==================================          INSERT/FIND          ===================================
   * ====================================================================================================
   * ====================================================================================================
   */

  uint32_t MysqlUserDatabase::insertUser (User & p) {
    auto req = this-> _client-> prepare ("INSERT INTO users (login, password) values (?, ?)");
    auto logLen = strnlen (p.login, LOGIN_LEN);
    auto pasLen = strnlen (p.password, PASSWORD_LEN);
    req-> setParam (0, p.login, logLen);
    req-> setParam (1, p.password, pasLen);

    req-> finalize ();
    req-> execute ();
    return req-> getGeneratedId ();
  }

  bool MysqlUserDatabase::findByLogin (const std::string & login, User & u) {
    if (this-> findByLoginInCache (login, u)) return true;

    ::memset (&u, 0, sizeof (User));
    auto logLen = std::min (login.length (), (uint64_t) LOGIN_LEN);
    ::memcpy (u.login, login.c_str (), logLen);

    auto req = this-> _client-> prepare ("SELECT id, password from users where login=?");
    req-> setParam (0, u.login, logLen);
    req-> setResult (0, &u.id);
    req-> setResult (1, u.password, PASSWORD_LEN);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      this-> insertByLoginInCache (u);
      return true;
    }

    return false;
  }

  bool MysqlUserDatabase::findById (uint32_t & id, User & u) {
    if (this-> findByIdInCache (id, u)) return true;

    ::memset (&u, 0, sizeof (User));
    u.id = id;

    auto req = this-> _client-> prepare ("SELECT login, password from users where id=?");
    req-> setParam (0, &u.id);
    req-> setResult (0, u.login, LOGIN_LEN);
    req-> setResult (1, u.password, PASSWORD_LEN);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      this-> insertByIdInCache (u);
      return true;
    }

    return false;
  }

  void MysqlUserDatabase::dispose () {
    this-> _client-> dispose ();
    this-> _client.reset ();
    UserDatabase::dispose ();
  }

}
