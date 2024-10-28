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

  void ShortUrlDatabase::configure (const rd_utils::utils::config::ConfigNode & conf) {
    auto dbName = conf ["db"].getOr ("name", "socialNet");
    auto dbAddr = conf ["db"].getOr ("addr", "localhost");
    auto dbUser = conf ["db"].getOr ("user", "root");
    auto dbPass = conf ["db"].getOr ("pass", "root");

    this-> _client = std::make_shared <socialNet::utils::MysqlClient> (dbAddr, dbUser, dbName);
    this-> _client-> connect (dbPass);
    this-> createTables ();
  }

  void ShortUrlDatabase::createTables () {
    auto reqSh = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS short_url (\n"
                                           "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                           "sh VARCHAR (16) NOT NULL,\n"
                                           "ln VARCHAR (255) NOT NULL\n)");
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
    req-> setParam (0, p.sh.data (), p.sh.len ());
    req-> setParam (1, p.ln.data (), p.ln.len ());

    req-> finalize ();
    req-> execute ();

    auto shId = req-> getGeneratedId ();
    return shId;
  }

  bool ShortUrlDatabase::findUrl (ShortUrl & p) {
    auto req = this-> _client-> prepare ("SELECT sh from short_url where ln = ?");
    req-> setParam (0, p.ln.data (), p.ln.len ());
    req-> setResult (0, p.sh.data (), 16);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      p.sh.forceLen (req-> getResultLen (0));
      return true;
    }

    return false;
  }

  bool ShortUrlDatabase::findUrlFromShort (ShortUrl & p) {
    auto req = this-> _client-> prepare ("SELECT ln from short_url where sh = ?");
    req-> setParam (0, p.sh.data (), p.sh.len ());
    req-> setResult (0, p.ln.data (), 255);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      p.ln.forceLen (req-> getResultLen (0));
      return true;
    }

    return false;
  }


}

std::ostream& operator << (std::ostream & s, socialNet::short_url::ShortUrl & url) {
  s << "ShortUrl (" << url.sh.data () << ", " << url.ln.data () << ")";
  return s;
}
