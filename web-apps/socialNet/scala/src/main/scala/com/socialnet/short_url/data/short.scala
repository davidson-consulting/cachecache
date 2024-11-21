package com.socialnet.short_url.data.short

import com.socialnet.short_url.database._
import com.socialnet.short_url.cache._

case class ShortUrl (long: String, short: String)

object ShortUrlRegistry {

  def createTable (db: ShortUrlDatabase) = {
    val sql = "CREATE TABLE IF NOT EXISTS shorturls (\n"
    +   "id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\n"
    +   "long_name VARCHAR (255) NOT NULL,\n"
    +   "short_name VARCHAR (255) NOT NULL\n)"

    val statement = db.getConnection ().createStatement ()
    statement.executeUpdate (sql)
    statement.close ()
  }

}

class ShortUrlRegistry (TTL : Int, db : ShortUrlDatabase, cache : ShortUrlCache) {

  def insert (url : ShortUrl): Unit = {
    val statement = db.getConnection ().prepareStatement ("insert into shorturls (long_name, short_name) values (?, ?)")
    statement.setString (1, url.long)
    statement.setString (2, url.short)

    statement.executeUpdate ()
    statement.close ()
  }

  def findByShort (short: String): Option[ShortUrl] = {
    this.getInCacheByShort (short) match {
      case Some (l) => {
        return Some (ShortUrl (l, short));
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select short_name, long_name from shorturls where short_name=?")
        statement.setString (1, short)

        val res = statement.executeQuery ();
        while (res.next ()) {
          val ret = ShortUrl (res.getString ("long_name"), res.getString ("short_name"));
          this.insertInCache (ret.long, short);

          statement.close ();

          return Some (ret);
        }

        None
      }
    }
  }

  def findByLong (long: String): Option[ShortUrl] = {
    this.getInCacheByLong (long) match {
      case Some (s) => {
        return Some (ShortUrl (long, s));
      }
      case _ => {
        val statement = db.getConnection ().prepareStatement ("select short_name, long_name from shorturls where long_name=?")
        statement.setString (1, long)

        val res = statement.executeQuery ();
        while (res.next ()) {
          val ret = ShortUrl (res.getString ("long_name"), res.getString ("short_name"));
          this.insertInCache (long, ret.short)
          statement.close ();

          return Some (ret);
        }

        None
      }
    }
  }

  def getInCacheByLong (long : String): Option[String] = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val value = redis.get (s"urls:${long}_short");
      return if (value != null) {
        Some (value)
      } else {
        None
      };
    }

    None
  }

  def getInCacheByShort (short : String): Option[String] = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      val value = redis.get (s"urls:${short}_long");
      return if (value != null) {
        Some (value)
      } else {
        None
      };
    }

    None
  }

  def insertInCache (long : String, short : String) = {
    val redis = this.cache.getConnection ();
    if (redis != null) {
      try {
        redis.set (s"urls:${long}_short", short)
        redis.set (s"urls:${short}_long", long)

        redis.expire (s"urls:${long}_short", this.TTL)
        redis.expire (s"urls:${short}_long", this.TTL)
      } catch {
        case e => {}
      }
    }
  }

}
