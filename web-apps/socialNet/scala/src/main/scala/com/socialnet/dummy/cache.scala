package com.socialnet.dummy.cache

import com.socialnet.utils.database._
import com.socialnet.master.options._
import com.socialnet.utils.cache._

import redis.clients.jedis._

class DummyCache (addr : String, port : Int)
    extends RedisCache (addr, port)
{

  override def postConnection () = {
    println ("connected")
  }

  def insert (key : String, value : String) = {
    if (this.connection != null) {
      this.connection.set (s"${key}_user", value);
    }
  }

  def get (key : String): Option[String] = {
    if (this.connection != null) {
      Some (this.connection.get (s"${key}_user"))
    } else {
      None
    }
  }

}
