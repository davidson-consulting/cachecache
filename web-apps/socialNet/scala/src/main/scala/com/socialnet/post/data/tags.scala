package com.socialnet.post.data.tags

import com.socialnet.post.database._
import com.socialnet.post.cache._
import scala.collection.mutable.ListBuffer
import com.socialnet.utils.serializer._

import java.sql.{Statement, PreparedStatement}

object UserTagsRegistry {

  def createTable (db: PostDatabase) = {
    var sql = "CREATE TABLE IF NOT EXISTS tags (\n"
    +   "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
    +   "user_id INT NOT NULL,\n"
    +   "post_id INT NOT NULL\n,"
    +   "FOREIGN KEY (post_id) REFERENCES post(id)\n)"

    val statement = db.getConnection ().createStatement ()
    statement.executeUpdate (sql)

    statement.close ()
  }

}


class UserTagsRegistry (TTL : Int, db : PostDatabase, cache : PostCache) {

  /**
    * Insert a list of tags for a given postId
    */
  def insert (userId: List[Int], postId: Int): Unit = {
    val statement = db.getConnection ().prepareStatement ("insert into tags (user_id, post_id) values (?, ?)");
    statement.setInt (2, postId)

    for (u <- userId) {
      statement.setInt (1, u)
      statement.addBatch ()
    }

    statement.executeBatch ()
    statement.close ()
  }

  /**
    * Find a list of user tags from a post Id
    */
  def find (postId: Int): List[Int] = {
    this.getInCache (postId) match {
      case Some (lst) => {
        lst
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select * from tags where post_id=?")
        statement.setInt (1, postId);

        val res = statement.executeQuery ();
        var result = new ListBuffer[Int]()
        while (res.next ()) {
          result += res.getInt ("user_id")
        }

        statement.close ()

        this.insertInCache (postId, result.toList);
        result.toList
      }
    }
  }

  def insertInCache (key : Int, value : List[Int]) = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      try {
        val sVal = CacheSerial.serialize (value);
        redis.set (s"tags:${key}", sVal);
        redis.expire (s"tags:${key}", this.TTL)
      } catch {
        case e => {  }
      }
    }
  }

  def getInCache (key : Int): Option[List[Int]] = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val value = redis.get (s"tags:${key}");
      return if (value != null) {
        CacheSerial.deserialize (value)
      } else {
        None
      };
    }

    None
  }

}
