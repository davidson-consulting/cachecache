package com.socialnet.short_url.service

import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill  }

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.utils.request._
import com.socialnet.utils.service._
import com.socialnet.utils.response._

import com.socialnet.short_url.protocol._
import com.socialnet.short_url.database._
import com.socialnet.short_url.cache._
import com.socialnet.short_url.data.short._

import java.util.regex.{Matcher, Pattern}
import java.util.{Random};


class ShortUrlServiceActor (
  name : String,
  config: ServiceConfig,
  rqtCacheConfig : CacheConfig,
  cacheConfig : CacheConfig,
  dbConfig : DBConfig,
  registryConfig: RegistryConfig
)
    extends ServiceActor (ShortUrlService.NAME, registryConfig, rqtCacheConfig)
{

  var database : ShortUrlDatabase = null
  var cache : ShortUrlCache = null

  var shortReg : ShortUrlRegistry = null

  var random : Random = null

  override def postActivation (): Unit = {
    this.database = new ShortUrlDatabase (this.dbConfig)
    this.cache = new ShortUrlCache (this.cacheConfig)
    this.database.connect ()
    this.cache.connect ()

    this.random = new Random (System.currentTimeMillis ())

    val ttl = if (this.config.params.contains ("cache-TTL")) {
      this.config.params ("cache-TTL").asInstanceOf [Int]
    } else {
      cacheConfig.TTL
    }

    this.shortReg = new ShortUrlRegistry (ttl, this.database, this.cache)
  }

  override def processMessage (msg: Any, replyTo: ActorRef): Unit = {
    msg match {
      case LongToShortReq (long) => // request to shorten a url
        this.shortenUrl (long, replyTo)

      case ShortToLongReq (short) => // request to find the long version of a short url
        this.retreiveUrl (short, replyTo)

      case e =>
        log.error (s"Unmanaged message $e")
    }
  }

  /**
    * Answering a LongToShortReq
    * @params:
    *    - long: the long url to shorten
    *    - replyTo: the actor requesting the operation
    */
  def shortenUrl (url: String, replyTo: ActorRef): Unit = {
    this.shortReg.findByLong (url) match {
      case Some (short) =>
        replyTo ! LongToShortResp (short.short)
      case _ =>
        val short = this.createShort ()
        this.shortReg.insert (ShortUrl (url, short))

        replyTo ! LongToShortResp (short)
    }
  }

  /**
    * Answering a ShortToLongReq
    * @params:
    *    - url: the short url to find
    *    - replyTo: the actor requesting the operation
    */
  def retreiveUrl (url: String, replyTo: ActorRef): Unit = {
    this.shortReg.findByShort (url) match {
      case Some (long) =>
        replyTo ! ShortToLongResp (Some (long.long))
      case _ =>
        replyTo ! ShortToLongResp (None)
    }
  }

  /**
    * Create a short url (and supposedly uniq?)
    */
  def createShort (): String = {
    val char_map = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    var result = new StringBuilder (this.id)

    result ++= "_"
    for (i <- 0 until 10) {
      val j = Math.abs (this.random.nextInt ()) % char_map.size
      result += char_map (j)
    }

    result.toString
  }


}
