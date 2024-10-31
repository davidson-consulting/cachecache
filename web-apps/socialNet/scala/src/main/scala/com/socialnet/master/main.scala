package com.socialnet.master.main

import scala.Array;
import akka.actor.{ Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill }

import com.socialnet.text.service.{ TextServiceActor }
import com.socialnet.dummy.service.{ DummyServiceActor }

import com.typesafe.config.ConfigFactory

import com.socialnet.compose.service._
import com.socialnet.text.service._
import com.socialnet.users.service._
import com.socialnet.short_url.service._
import com.socialnet.social_graph.service._
import com.socialnet.timeline.service._
import com.socialnet.post.service._
import com.socialnet.master.options._
import com.socialnet.utils.configuration._

object Main {

  def getCacheConfig (name: String, config: Config): CacheConfig = {
    if (config.caches.contains (name)) {
      config.caches (name)
    } else {
      CacheConfig (null, null, 0, 0)
    }
  }

  def getDBConfig (name: String, config: Config): DBConfig = {
    if (config.dbs.contains (name)) {
      config.dbs (name)
    } else {
      DBConfig (null, null, null, null, 0)
    }
  }

  def propActor (service: (String, ServiceConfig), config: Config) : Option [Props] = {
    service._2.service match {
      case "ComposeService" =>
        return Some (
          Props (new ComposeServiceActor (service._1,
            service._2,
            getCacheConfig (service._2.rqt_cache, config),
            getCacheConfig (service._2.cache, config),
            config.registry))
        )
      case "TextService" =>
        return Some (Props (new TextServiceActor (service._1, service._2,
          getCacheConfig (service._2.rqt_cache, config),
          config.registry)))
      case "DummyService" =>
        return Some (Props (
          new DummyServiceActor (service._1, service._2,
            getCacheConfig (service._2.rqt_cache, config),
            getCacheConfig (service._2.cache, config),
            getDBConfig (service._2.db, config), config.registry)
        ))
      case "UserService" =>
        return Some (Props (
          new UserServiceActor (service._1, service._2,
            getCacheConfig (service._2.rqt_cache, config),
            getCacheConfig (service._2.cache, config),
            getDBConfig (service._2.db, config), config.registry)
        ))
      case "ShortUrlService" =>
        return Some (Props (
          new ShortUrlServiceActor (service._1, service._2,
            getCacheConfig (service._2.rqt_cache, config),
            getCacheConfig (service._2.cache, config),
            getDBConfig (service._2.db, config), config.registry)
        ))
      case "SocialGraphService" =>
        return Some (Props (
          new SocialGraphServiceActor (service._1, service._2,
            getCacheConfig (service._2.rqt_cache, config),
            getCacheConfig (service._2.cache, config),
            getDBConfig (service._2.db, config), config.registry)
        ))
      case "TimelineService" =>
        return Some (Props (
          new TimelineServiceActor (service._1, service._2,
            getCacheConfig (service._2.rqt_cache, config),
            getCacheConfig (service._2.cache, config),
            getDBConfig (service._2.db, config), config.registry)
        ))
      case "PostStorageService" =>
        return Some (Props (
          new PostStorageServiceActor (service._1, service._2,
            getCacheConfig (service._2.rqt_cache, config),
            getCacheConfig (service._2.cache, config),
            getDBConfig (service._2.db, config), config.registry)
        ))
      case x => {
        println ("Unknown actor type : " + x)
        None
      }
    }
  }

  def main (args: Array[String]) = {
    val actorConfig = Options.parseOptions (args)
    val sysConfig = ConfigFactory.parseString (
      ConfigLoader.readConfiguration ("resources/master.conf",
        Map ("addr"-> actorConfig.addr, "port"-> actorConfig.port.toString)
      )
    );

    val system = ActorSystem ("RemoteSystem", sysConfig)
    for (service <- actorConfig.services) {
      for (i <- 0 until service._2.replicate) {
        this.propActor (service, actorConfig)  match {
          case Some (x) =>
            system.actorOf (x, name=service._1 + i)
          case None =>
            println ("Sniff..")
        }
      }
    }


    Runtime.getRuntime().addShutdownHook(new Thread {
      override def run = {
        system.actorSelection("/user/*") ! PoisonPill
      }
    })
  }

}
