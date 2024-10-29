package com.socialnet.users.data.users

import com.socialnet.users.database._
import com.socialnet.users.cache._
import com.socialnet.utils.serializer._

case class User (id: Int, login: String, password: String)

object UserRegistry {

  def createTable (db: UserDatabase) = {
    val sql = "CREATE TABLE IF NOT EXISTS users (\n"
    +   "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
    +   "login VARCHAR (255) NOT NULL,\n"
    +   "password VARCHAR (255) NOT NULL\n)"

    val statement = db.getConnection ().createStatement ()
    statement.executeUpdate (sql)
    statement.close ()
  }

}

class UserRegistry (TTL : Int, db : UserDatabase, cache : UserCache) {

  def insert (user : User) = {
    if (user.id == 0) {
      val statement = db.getConnection ().prepareStatement ("insert into users (login, password) values (?, ?)")
      statement.setString (1, user.login)
      statement.setString (2, user.password)

      statement.executeUpdate ()
      statement.close ();
    } else {
      val statement = db.getConnection ().prepareStatement ("insert into users values (?, ?, ?)")
      statement.setInt (1, user.id)
      statement.setString (2, user.login)
      statement.setString (3, user.password)

      statement.executeUpdate ()
      statement.close ();
    }
  }

  def findByLogin (login: String): Option[User] = {
    this.getByLoginInCache (login) match {
      case Some (user) => {
        return Some (user);
      }
      case _ => {

        val statement = db.getConnection ().prepareStatement ("select * from users where login=?")
        statement.setString (1, login)

        val res = statement.executeQuery ();
        while (res.next ()) {
          val ret = User (res.getInt ("id"), res.getString ("login"), res.getString ("password"));
          this.insertInCache (ret);

          statement.close ()

          return Some (ret);
        }

        None
      }
    }
  }

  def findByLogins (logins: List[String]): List[User] = {
    val statement = db.getConnection ().prepareStatement ("select * from users where login=?")
    var list: List[User] = List()
    for (l <- logins) {
      statement.setString (1, l)

      val res = statement.executeQuery ();
      while (res.next ()) {
        val user = User (res.getInt ("id"), res.getString ("login"), res.getString ("password"));
        list ++= List (user);

        this.insertInCache (user);
      }
    }

    statement.close ()
    list
  }

  def findById (id: Int): Option[User] = {
    this.getByIdInCache (id) match {
      case Some (u) => {
        return Some (u);
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select * from users where id=?")
        statement.setInt (1, id)

        val res = statement.executeQuery ();
        while (res.next ()) {
          val ret = User (res.getInt ("id"), res.getString ("login"), res.getString ("password"));
          this.insertInCache (ret);

          statement.close ()
          return Some (ret);
        }

        None
      }
    }
  }

  def findByIds (ids: List[Int]): List[User] = {
    val statement = db.getConnection ().prepareStatement ("select * from users where id=?")
    var list: List[User] = List()
    for (l <- ids) {
      statement.setInt (1, l)

      val res = statement.executeQuery ();
      while (res.next ()) {
        val u = User (res.getInt ("id"), res.getString ("login"), res.getString ("password"));
        list ++= List (u);
        this.insertInCache (u);
      }
    }

    statement.close ()
    list
  }


  /**
    ========================================================================================
    ========================================================================================
    ===============================    CACHE MANAGEMENT    =================================
    ========================================================================================
    ========================================================================================
    */

  def getByLoginInCache (login : String): Option[User] = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val u = redis.get (s"user:${login}_login")
      return if (u != null) {
        CacheSerial.deserialize [User] (u)
      } else {
        None
      }
    }

    None
  }

  def getByIdInCache (id : Int): Option[User] = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val u = redis.get (s"user:${id}_id")
      return if (u != null) {
        CacheSerial.deserialize [User] (u)
      } else {
        None
      }
    }

    None
  }

  def insertInCache (user : User) = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val us = CacheSerial.serialize (user)
      try {
        redis.set (s"user:${user.login}_login", us)
        redis.set (s"user:${user.id}_id", us)

        redis.expire (s"user:${user.login}_login", this.TTL)
        redis.expire (s"user:${user.id}_id", this.TTL)
      } catch {
        case e => {}
      }
    }
  }

}
