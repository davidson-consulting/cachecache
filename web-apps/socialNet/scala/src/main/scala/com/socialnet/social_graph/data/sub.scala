package com.socialnet.social_graph.data.sub

import com.socialnet.social_graph.database._
import com.socialnet.social_graph.cache._
import com.socialnet.utils.serializer._

import scala.collection.mutable.ListBuffer
import scala.jdk.CollectionConverters._

case class Sub (userId: Int, toWhom: Int)

object SubRegistry {

  def createTable (db: SocialGraphDatabase) = {
    val sql = "CREATE TABLE IF NOT EXISTS subs (\n"
    +   "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
    +   "user_id INT NOT NULL,\n"
    +   "to_whom INT NOT NULL\n)"

    val statement = db.getConnection ().createStatement ()
    statement.executeUpdate (sql)
    statement.close ();
  }


}

class SubRegistry (TTL : Int, db : SocialGraphDatabase, cache : SocialGraphCache) {

  def insertIfNotExist (sub: Sub): Unit = {
    val statement = db.getConnection ().prepareStatement ("select * from subs where user_id=? AND to_whom=?")
    statement.setInt (1, sub.userId)
    statement.setInt (2, sub.toWhom)

    val res = statement.executeQuery ()
    if (!res.next ()) {
      this.insert (sub)
    }

    statement.close ();
  }

  def find (sub: Sub): Option[Sub] = {
    val statement = db.getConnection ().prepareStatement ("select user_id, to_whom from subs where user_id=? and to_whom=?")
    statement.setInt (1, sub.userId)
    statement.setInt (2, sub.toWhom)

    val res = statement.executeQuery ()
    val ret = if (res.next ()) {

      Some (Sub (res.getInt ("user_id"), res.getInt ("to_whom")))
    } else {
      None
    }

    statement.close ()
    ret
  }

  def insert (sub: Sub): Unit = {
    val statement = db.getConnection ().prepareStatement ("insert into subs (user_id, to_whom) values (?, ?)")
    statement.setInt (1, sub.userId)
    statement.setInt (2, sub.toWhom)

    statement.executeUpdate ()
    statement.close ()

    this.removeInCache (sub.userId, sub.toWhom);
  }

  def findByUserId (userId: Int, page: Int, nb: Int): List[Sub] = {
    this.getSubscriptionsInCache (userId, page, nb) match {
      case Some (lst) => {
        lst
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select user_id, to_whom from subs where user_id=? order by id DESC limit ? offset ?")
        statement.setInt (1, userId)
        statement.setInt (2, nb)
        statement.setInt (3, page * nb)

        var lst = new ListBuffer[Sub]()
        val res = statement.executeQuery ()
        while (res.next ()) {
          lst += Sub (res.getInt ("user_id"), res.getInt ("to_whom"))
        }

        statement.close ()
        val lstL = lst.toList;
        this.insertSubscriptionsInCache (userId, page, nb, lstL);

        lstL
      }
    }
  }

  def findByToWhom (userId: Int, page: Int, nb: Int): List[Sub] = {
    this.getFollowersInCache (userId, page, nb) match {
      case Some (lst) => {
        lst
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select user_id, to_whom from subs where to_whom=? order by id DESC limit ? offset ?")
        statement.setInt (1, userId)
        statement.setInt (2, nb)
        statement.setInt (3, page * nb)

        var lst = new ListBuffer[Sub]()
        val res = statement.executeQuery ()
        while (res.next ()) {
          lst += Sub (res.getInt ("user_id"), res.getInt ("to_whom"))
        }

        val lstL = lst.toList
        statement.close ()

        this.insertFollowersInCache (userId, page, nb, lstL)
        lstL
      }
    }
  }

  def countNbSubscriptions (userId: Int): Int = {
    this.getCountSubsInCache (userId) match {
      case Some (c) => {
        c
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select count(*) as cnt from subs where user_id=?");
        statement.setInt (1, userId);

        val res = statement.executeQuery ()
        val ret = if (res.next ()) {
          res.getInt ("cnt")
        } else {
          0
        }

        statement.close ()
        this.insertCountSubInCache (userId, ret)
        ret
      }
    }
  }

  def countNbFollowers (userId: Int): Int = {
    this.getCountFollowsInCache (userId) match {
      case Some (c) => {
        c
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select count(*) as cnt from subs where to_whom=?");
        statement.setInt (1, userId);

        val res = statement.executeQuery ()
        val ret = if (res.next ()) {
          res.getInt ("cnt")
        } else {
          0
        }

        statement.close ()
        this.insertCountFollowsInCache (userId, ret)
        ret
      }
    }
  }

  def delete (sub: Sub): Unit = {
    val statement = db.getConnection ().prepareStatement ("delete from subs where user_id=? and to_whom=?")
    statement.setInt (1, sub.userId)
    statement.setInt (2, sub.toWhom)

    statement.executeUpdate ()
    statement.close ()
    this.removeInCache (sub.userId, sub.toWhom)
  }


  /**
    ========================================================================================
    ========================================================================================
    ===============================    CACHE MANAGEMENT    =================================
    ========================================================================================
    ========================================================================================
   */

  def removeInCache (userId : Int, toWhom : Int) = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val subs = redis.keys (s"subs:${userId}_subs/*/*")
      val foll = redis.keys (s"subs:${toWhom}_foll/*/*")

      for (k <- subs.asScala) {
        redis.del (k)
      }
      for (k <- foll.asScala) {
        redis.del (k)
      }

      redis.del (s"subs:${userId}_cnt_subs")
      redis.del (s"subs:${toWhom}_cnt_foll")
    }
  }

  def getSubscriptionsInCache (userId : Int, page : Int, nb : Int): Option[List[Sub]] = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      val subs = redis.get (s"subs:${userId}_subs/${page}/${nb}")
      if (subs != null) {
        return CacheSerial.deserialize [List[Sub]] (subs)
      }
    }
    None
  }

  def insertSubscriptionsInCache (userId : Int, page : Int, nb : Int, lst : List[Sub]) = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      val subs = CacheSerial.serialize (lst)

      try {
        redis.set (s"subs:${userId}_subs/${page}/${nb}", subs);
        redis.expire (s"subs:${userId}_subs/${page}/${nb}", this.TTL)
      } catch {
        case e => {}
      }
    }
  }

  def getFollowersInCache (userId : Int, page : Int, nb : Int): Option[List[Sub]] = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val subs = redis.get (s"subs:${userId}_foll/${page}/${nb}")
      if (subs != null) {
        return CacheSerial.deserialize [List[Sub]] (subs)
      }
    }

    None
  }

  def insertFollowersInCache (userId : Int, page : Int, nb : Int, lst : List[Sub]) = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      val subs = CacheSerial.serialize (lst)

      try {
        redis.set (s"subs:${userId}_foll/${page}/${nb}", subs);
        redis.expire (s"subs:${userId}_foll/${page}/${nb}", this.TTL)
      } catch {
        case e => {}
      }
    }
  }


  def getCountSubsInCache (userId : Int): Option[Int] = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      val cnt = redis.get (s"subs:${userId}_cnt_subs")
      return if (cnt != null) {
        Some (cnt.toInt)
      } else {
        None
      }
    }
    None
  }

  def insertCountSubInCache (userId : Int, nb : Int) = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      try {
        redis.set (s"subs:${userId}_cnt_subs", s"${nb}")
        redis.expire (s"subs:${userId}_cnt_subs", this.TTL)
      } catch {
        case e => {}
      }
    }
  }

  def getCountFollowsInCache (userId : Int): Option[Int] = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      val cnt = redis.get (s"subs:${userId}_cnt_foll")
      return if (cnt != null) {
        Some (cnt.toInt)
      } else {
        None
      }
    }

    None
  }

  def insertCountFollowsInCache (userId : Int, nb : Int) = {
    val redis = this.cache.getConnection ()
    if (redis != null) {
      try {
        redis.set (s"subs:${userId}_cnt_foll", s"${nb}")
        redis.expire (s"subs:${userId}_cnt_foll", this.TTL)
      } catch {
        case e => {}
      }
    }
  }

}
