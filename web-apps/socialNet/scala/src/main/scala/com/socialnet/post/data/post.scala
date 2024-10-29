package com.socialnet.post.data.post

import com.socialnet.post.database._
import com.socialnet.post.cache._
import scala.collection.mutable.ListBuffer
import com.socialnet.utils.serializer._

import java.sql.{Statement, PreparedStatement}

object PostRegistry {

  def createTable (db : PostDatabase) = {
    var sql = "CREATE TABLE IF NOT EXISTS post (\n"
    +   "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
    +   "user_id INT NOT NULL,\n"
    +   "user_login VARCHAR(255) NOT NULL,\n"
    +   "text LONGTEXT NOT NULL\n)"

    val statement = db.getConnection ().createStatement ()
    statement.executeUpdate (sql)
    statement.close ()
  }

}


class PostRegistry (TTL : Int, db : PostDatabase, cache : PostCache) {

  /**
    * Insert a new post in the database with caching
    * @returns: the id of the inserted post
    */
  def insert (userId: Int, userLogin : String, text: String): Int = {
    val statement = this.db.getConnection ().prepareStatement ("insert into post (user_id, user_login, text) values (?, ?, ?)", Statement.RETURN_GENERATED_KEYS);

    statement.setInt (1, userId)
    statement.setString (2, userLogin)
    statement.setString (3, text)

    statement.executeUpdate ()

    val genKeys = statement.getGeneratedKeys ();
    val result = if (genKeys.next ()) {
      val i = genKeys.getInt (1);
      i
    } else {
      0
    }

    statement.close ()
    result
  }

  /**
    * Find in the db with the cache
    */
  def find (postId: Int): Option[(Int, String, String)] = {
    this.getInCache (postId) match {
      case Some (value) => {
        return Some (value);
      }
      case _ => {
        val statement = this.db.getConnection ().prepareStatement ("select * from post where id=?")
        statement.setInt (1, postId);

        val res = statement.executeQuery ();
        val result = if (res.next ()) {
          val value = (res.getInt ("user_id"), res.getString ("user_login"), res.getString ("text"))
          this.insertInCache (postId, value)

          Some (value)
        } else {
          None
        }

        statement.close ()
        result
      }
    }
  }

  def insertInCache (key : Int, value : (Int, String, String)) = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      try {
        val sVal = CacheSerial.serialize (value);
        redis.set (s"post:${key}", sVal);
        redis.expire (s"post:${key}", this.TTL);
      } catch {
        case e => {  } // might fail, if there are no memory left in cache
      }
    }
  }

  def getInCache (key : Int): Option[(Int, String, String)] = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val id = redis.get (s"post:${key}");
      return if (id != null) {
        CacheSerial.deserialize (id)
      } else {
        None
      }
    }

    None
  }

}
