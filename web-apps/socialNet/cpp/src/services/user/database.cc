#include "database.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::user {

  UserDatabase::UserDatabase () :
    _client (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void UserDatabase::configure (const config::ConfigNode & conf) {
    auto dbName = conf ["db"].getOr ("name", "socialNet");
    auto dbAddr = conf ["db"].getOr ("addr", "localhost");
    auto dbUser = conf ["db"].getOr ("user", "root");
    auto dbPass = conf ["db"].getOr ("pass", "root");

    this-> _client = std::make_shared <socialNet::utils::MysqlClient> (dbAddr, dbUser, dbPass, dbName);
    this-> _client-> connect ();
    this-> createTables ();
  }

  void UserDatabase::createTables () {
    auto reqUser = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS users (\n"
                                             "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                             "login VARCHAR(" + std::to_string (LOGIN_LEN) + ") NOT NULL,\n"
                                             "password VARCHAR(" + std::to_string (PASSWORD_LEN) + ") NOT NULL\n)"
                                             );
    reqUser-> finalize ();
    reqUser-> execute ();
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          INSERT/FIND         ========================================
   * ====================================================================================================
   * ====================================================================================================
   */

  uint32_t UserDatabase::insertUser (User & p) {
    auto req = this-> _client-> prepare ("INSERT INTO users (login, password) values (?, ?)");
    auto logLen = strnlen (p.login, LOGIN_LEN);
    auto pasLen = strnlen (p.password, PASSWORD_LEN);
    req-> setParam (0, p.login, logLen);
    req-> setParam (1, p.password, pasLen);

    req-> finalize ();
    req-> execute ();
    return req-> getGeneratedId ();
  }

  bool UserDatabase::findByLogin (const std::string & login, User & u) {
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
      return true;
    }

    return false;
  }

  bool UserDatabase::findById (uint32_t & id, User & u) {
    ::memset (&u, 0, sizeof (User));

    auto req = this-> _client-> prepare ("SELECT login, password from users where id=?");
    req-> setParam (0, &id);
    req-> setResult (0, u.login, LOGIN_LEN);
    req-> setResult (1, u.password, PASSWORD_LEN);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      return true;
    }

    return false;
  }

}
