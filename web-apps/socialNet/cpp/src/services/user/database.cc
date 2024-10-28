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

    this-> _client = std::make_shared <socialNet::utils::MysqlClient> (dbAddr, dbUser, dbName);
    this-> _client-> connect (dbPass);
    this-> createTables ();
  }

  void UserDatabase::createTables () {
    auto reqUser = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS users (\n"
                                             "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                             "login VARCHAR(16) NOT NULL,\n"
                                             "password VARCHAR(64) NOT NULL\n)"
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
    req-> setParam (0, p.login.data (), p.login.len ());
    req-> setParam (1, p.password.data (), p.password.len ());

    req-> finalize ();
    req-> execute ();
    return req-> getGeneratedId ();
  }

  bool UserDatabase::findByLogin (rd_utils::memory::cache::collection::FlatString <16> & login, User & u) {
    auto req = this-> _client-> prepare ("SELECT id, password from users where login=?");
    req-> setParam (0, login.data (), login.len ());
    req-> setResult (0, &u.id);
    req-> setResult (1, u.password.data (), 64);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      u.password.forceLen (req-> getResultLen (1));
      return true;
    }

    return false;
  }

  bool UserDatabase::findById (uint32_t & id, User & u) {
    auto req = this-> _client-> prepare ("SELECT login, password from users where id=?");
    req-> setParam (0, &id);
    req-> setResult (0, u.login.data (), 16);
    req-> setResult (1, u.password.data (), 64);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      u.login.forceLen (req-> getResultLen (0));
      u.password.forceLen (req-> getResultLen (1));
      return true;
    }
    std::cout << "Not found" << std::endl;

    return false;
  }

}
