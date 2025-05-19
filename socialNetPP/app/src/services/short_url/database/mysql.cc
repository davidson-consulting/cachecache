#include "mysql.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::short_url {

  MysqlShortUrlDatabase::MysqlShortUrlDatabase () :
    _client (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MysqlShortUrlDatabase::configure (const std::string & db, const std::string & ch, const config::ConfigNode & conf) {
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

  void MysqlShortUrlDatabase::createTables () {
    auto reqSh = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS short_url (\n"
                                           "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                           "sh VARCHAR (" + std::to_string (SHORT_LEN) + ") NOT NULL,\n"
                                           "ln VARCHAR (" + std::to_string (LONG_LEN) + ") NOT NULL\n)");
    reqSh-> finalize ();
    reqSh-> execute ();
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================              SH_URL             =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void MysqlShortUrlDatabase::insertUrl (ShortUrl & p) {
    auto req = this-> _client-> prepare ("INSERT INTO short_url (sh, ln) values (?, ?)");
    req-> setParam (0, p.sh, strnlen (p.sh, SHORT_LEN));
    req-> setParam (1, p.ln, strnlen (p.ln, LONG_LEN));

    req-> finalize ();
    req-> execute ();
  }

  bool MysqlShortUrlDatabase::findUrl (ShortUrl & p) {
    if (this-> findUrlFromLongInCache (p)) return true;

    auto req = this-> _client-> prepare ("SELECT sh from short_url where ln=?");
    req-> setParam (0, p.ln, strnlen (p.ln, LONG_LEN));
    req-> setResult (0, p.sh, SHORT_LEN);
    req-> finalize ();
    req-> execute ();

    if (req-> next ()) {
      this-> insertUrlInCache (p);
      return true;
    }

    return false;
  }

  bool MysqlShortUrlDatabase::findUrlFromShort (ShortUrl & p) {
    if (this-> findUrlFromShort (p)) return true;

    auto req = this-> _client-> prepare ("SELECT ln from short_url where sh=?");
    req-> setParam (0, p.sh, strnlen (p.sh, SHORT_LEN));
    req-> setResult (0, p.ln, LONG_LEN);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      this-> insertUrlInCache (p);
      return true;
    }

    return false;
  }

  void MysqlShortUrlDatabase::dispose () {
    this-> _client-> dispose ();
    this-> _client.reset ();
    ShortUrlDatabase::dispose ();
  }

}
