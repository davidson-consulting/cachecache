#include "database.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::short_url {

  ShortUrlDatabase::ShortUrlDatabase () :
    _client (nullptr)
  {}


  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ShortUrlDatabase::configure (const std::string & db, const std::string & ch, const config::ConfigNode & conf) {
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

  void ShortUrlDatabase::createTables () {
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

  uint32_t ShortUrlDatabase::insertUrl (ShortUrl & p) {
    auto req = this-> _client-> prepare ("INSERT INTO short_url (sh, ln) values (?, ?)");
    req-> setParam (0, p.sh, strnlen (p.sh, SHORT_LEN));
    req-> setParam (1, p.ln, strnlen (p.ln, LONG_LEN));

    req-> finalize ();
    req-> execute ();

    auto shId = req-> getGeneratedId ();
    return shId;
  }

  bool ShortUrlDatabase::findUrl (ShortUrl & p) {
    auto req = this-> _client-> prepare ("SELECT sh from short_url where ln=?");
    req-> setParam (0, p.ln, strnlen (p.ln, LONG_LEN));
    req-> setResult (0, p.sh, SHORT_LEN);
    req-> finalize ();
    req-> execute ();

    if (req-> next ()) {
      return true;
    }

    return false;
  }

  bool ShortUrlDatabase::findUrlFromShort (ShortUrl & p) {
    auto req = this-> _client-> prepare ("SELECT ln from short_url where sh=?");
    req-> setParam (0, p.sh, strnlen (p.sh, SHORT_LEN));
    req-> setResult (0, p.ln, LONG_LEN);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      return true;
    }

    return false;
  }

}

std::ostream& operator << (std::ostream & s, socialNet::short_url::ShortUrl & url) {
  s << "ShortUrl (" << url.sh << ", " << url.ln << ")";
  return s;
}
