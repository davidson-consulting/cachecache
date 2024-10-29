package com.socialnet.dummy.service

import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill  }

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.utils.request._
import com.socialnet.text.protocol._
import com.socialnet.users.protocol._
import com.socialnet.compose.protocol._
import com.socialnet.dummy.database._
import com.socialnet.dummy.cache._

import com.socialnet.utils.service._
import com.socialnet.utils.response._

import com.socialnet.utils.jwt_tokens._


class DummyServiceActor (
  name : String,
  config: ServiceConfig,
  rqtCacheConfig : CacheConfig,
  cacheConfig : CacheConfig,
  dbConfig : DBConfig, registryConfig: RegistryConfig)
    extends ServiceActor ("DummyService", registryConfig, rqtCacheConfig)
{

  val msg = "Message for @admin, @user, this is an url https://mail.google.com/mail/u/0/ for you and https://stackoverflow.com/questions/39505725/long-int-in-scala-to-hex-string"
  var cache : DummyCache = null

  override def postActivation() : Unit = {
    context.self ! "Run !"


    this.cache = new DummyCache (this.cacheConfig.addr, this.cacheConfig.port)
    this.cache.connect ()
  }

  override def processMessage (msg : Any, replyTo : ActorRef): Unit = {
    msg match {
      case e =>
        this.cache.insert ("Test", "test");
        for (i <- 0 until 10) {
          this.cache.get ("Test") match {
            case Some (v) =>
              println ("V : " + v)
            case _ =>
              println ("Not found ?")
          }
        }
    }
  }

}
