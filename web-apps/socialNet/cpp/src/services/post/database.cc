#include "database.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace socialNet::post {

  PostDatabase::PostDatabase () :
    _client (nullptr)
  {}

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          CONFIGURATION         ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void PostDatabase::configure (const rd_utils::utils::config::ConfigNode & conf) {
    auto dbName = conf ["db"].getOr ("name", "socialNet");
    auto dbAddr = conf ["db"].getOr ("addr", "localhost");
    auto dbUser = conf ["db"].getOr ("user", "root");
    auto dbPass = conf ["db"].getOr ("pass", "root");

    this-> _client = std::make_shared <socialNet::utils::MysqlClient> (dbAddr, dbUser, dbName);
    this-> _client-> connect (dbPass);
    this-> createTables ();
  }

  void PostDatabase::createTables () {
    auto reqPost = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS post (\n"
                                             "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                             "user_id INT NOT NULL,\n"
                                             "user_login VARCHAR (16) NOT NULL,\n"
                                             "text VARCHAR (280) NOT NULL\n)");
    reqPost-> finalize ();
    reqPost-> execute ();

    auto reqTags = this-> _client-> prepare ("CREATE TABLE IF NOT EXISTS tags (\n"
                                             "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
                                             "user_id INT NOT NULL,\n"
                                             "post_id INT NOT NULL\n,"
                                             "FOREIGN KEY (post_id) REFERENCES post(id)\n)");
    reqTags-> finalize ();
    reqTags-> execute ();
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================              POSTS             ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  uint32_t PostDatabase::insertPost (Post & p) {
    auto req = this-> _client-> prepare ("INSERT INTO post (user_id, user_login, text) values (?, ?, ?)");
    req-> setParam (0, &p.userId);
    req-> setParam (1, p.userLogin, strnlen (p.userLogin, 16));
    req-> setParam (2, p.text, strnlen (p.text, 512));

    req-> finalize ();
    req-> execute ();

    auto postId = req-> getGeneratedId ();
    req = nullptr;

    this-> insertTags (postId, p.tags, p.nbTags);
    return postId;
  }

  bool PostDatabase::findPost (uint32_t id, Post & p) {
    auto req = this-> _client-> prepare ("SELECT user_id, user_login, text FROM post where id=?");
    req-> setParam (0, &id);
    req-> setResult (0, &p.userId);
    req-> setResult (1, p.userLogin, 16);
    req-> setResult (2, p.text, 511);

    req-> finalize ();
    req-> execute ();
    if (req-> next ()) {
      p.userLogin [req-> getResultLen (1)] = 0;
      p.text [req-> getResultLen (2)] = 0;
      req = nullptr;

      this-> findTags (id, p.tags, p.nbTags);

      return true;
    }

    return false;
  }

  /**
   * ====================================================================================================
   * ====================================================================================================
   * ==============================              TAGS             =======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void PostDatabase::insertTags (uint32_t postId, uint32_t * tags, uint8_t nbTags) {
    auto req = this-> _client-> prepare ("INSERT INTO tags (post_id, user_id) values (?, ?)");
    uint32_t tagId;
    req-> setParam (0, &postId);
    req-> setParam (1, &tagId);
    req-> finalize ();

    for (uint32_t i = 0 ; i < nbTags && i < 16; i++) {
      tagId = tags [i];
      req-> execute ();
    }
  }

  void PostDatabase::findTags (uint32_t postId, uint32_t * tags, uint8_t & nbTags) {
    auto req = this-> _client-> prepare ("SELECT user_id FROM tags where post_id=?");
    uint32_t tagId;

    req-> setParam (0, &postId);
    req-> setResult (0, &tagId);
    req-> finalize ();

    nbTags = 0;
    req-> execute ();
    while (req-> next () && nbTags < 1) {
      tags [nbTags] = tagId;
      nbTags += 1;
    }
  }

}


std::ostream& operator<< (std::ostream & s, const socialNet::post::Post & p) {
  s << "Post (" << p.userId << "," << p.userLogin << "," << p.text << ", [";
  for (uint32_t i = 0 ; i < p.nbTags ; i++) {
    if (i != 0) s << ", ";
    s << p.tags [i];
  }
  s << "])";
  return s;
}
