package com.socialnet.timeline.data.user

import com.socialnet.timeline.database._
import com.socialnet.timeline.cache._
import com.socialnet.utils.serializer._

import com.socialnet.timeline.data.post._
import scala.collection.mutable.ListBuffer
import scala.jdk.CollectionConverters._

import java.sql.{Statement, PreparedStatement}

case class UserInsertStatement (
  var currentIndex: Int = 0,
  var count: Int = 0,
  var statement: PreparedStatement = null
)


object UserTimelineRegistry {

  def createTable (db: TimelineDatabase) = {
    var sql = "CREATE TABLE IF NOT EXISTS user_timeline (\n"
    +   "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
    +   "user_id INT NOT NULL,\n"
    +   "post_id INT NOT NULL\n)"

    val statement = db.getConnection ().createStatement ()
    statement.executeUpdate (sql)
    statement.close ()
  }

}
class UserTimelineRegistry (TTL : Int, db : TimelineDatabase, cache : TimelineCache) {

  def insertBatch (post: Post, result : UserInsertStatement): Unit = {
    if (result.statement == null) {
      result.statement = db.getConnection ().prepareStatement ("insert into user_timeline (user_id, post_id) values (?, ?)");
      result.currentIndex = 0;
    }

    result.statement.setInt (1, post.userId)
    result.statement.setInt (2, post.postId)
    result.statement.addBatch ();

    result.count += 1;
    result.currentIndex += 1;

    if (result.currentIndex == 100) {
      result.currentIndex = 0
      result.statement.executeBatch ()
    }

    this.removeInCache (post.userId);
  }

  def select (userId: Int, page : Int, nb : Int): List[Post] = {
    this.getInCache (userId, page, nb) match {
      case Some (lst) => {
        return lst;
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select * from user_timeline where user_id=? order by id DESC limit ? offset ?")
        statement.setInt (1, userId);
        statement.setInt (2, nb);
        statement.setInt (3, nb * page);

        val res = statement.executeQuery ();
        var lst = new ListBuffer [Post]()
        while (res.next ()) {
          lst += Post (res.getInt ("user_id"), res.getInt ("post_id"));
        }

        val lstL = lst.toList
        statement.close ()
        this.insertInCache (userId, page, nb, lstL);
        lstL
      }
    }
  }

  def count (userId: Int): Int = {
    this.getCountInCache (userId) match {
      case Some (c) => {
        return c;
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select count(*) as cnt from user_timeline where user_id=?")
        statement.setInt (1, userId);

        val res = statement.executeQuery ();
        val ret = if (res.next ()) {
          res.getInt ("cnt")
        } else {
          0
        }

        statement.close ()
        this.insertCountInCache (userId, ret);
        ret
      }
    }
  }

  /**
    ========================================================================================
    ========================================================================================
    ===============================    CACHE MANAGEMENT    =================================
    ========================================================================================
    ========================================================================================
    */

  def removeInCache (userId : Int) = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      val keys = redis.keys (s"time:${userId}_user/*/*")
      for (k <- keys.asScala) {
        redis.del (k)
      }

      redis.del (s"time:${userId}_cnt_user")
    }
  }

  def getInCache (userId : Int, page : Int, nb : Int): Option[List[Post]] = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val times = redis.get (s"time:${userId}_user/${page}/${nb}")
      if (times != null) {
        return CacheSerial.deserialize[List[Post]] (times)
      }
    }

    None
  }

  def insertInCache (userId : Int, page : Int, nb : Int, lst : List[Post]) = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      val times = CacheSerial.serialize (lst);

      try {
        redis.set (s"time:${userId}_user/${page}/${nb}", times);
        redis.expire (s"time:${userId}_user/${page}/${nb}", this.TTL)
      } catch {
        case e => {}
      }
    }
  }

  def getCountInCache (userId : Int): Option[Int] = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      val cnt = redis.get (s"time:${userId}_cnt_user")
      return if (cnt != null) {
        Some (cnt.toInt)
      } else {
        None
      }
    }
    None
  }

  def insertCountInCache (userId : Int, nb : Int) = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      try {
        redis.set (s"time:${userId}_cnt_user", s"${nb}")
        redis.expire (s"time:${userId}_cnt_user", this.TTL)
      } catch {
        case e => {}
      }
    }
  }

}
