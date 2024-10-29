package com.socialnet.registry.main

import scala.Array;
import akka.actor.{ Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill }
import com.typesafe.config.ConfigFactory

import com.socialnet.registry.service._
import com.socialnet.registry.options._
import com.socialnet.utils.configuration._


object Main {

  def main (args : Array[String]) = {
    val actorConfig = Options.parseOptions (args)
    val sysConfig = ConfigFactory.parseString (
      ConfigLoader.readConfiguration ("resources/registry.conf",
        Map ("addr"-> actorConfig.addr, "port"-> actorConfig.port.toString)
      )
    );
    val system = ActorSystem ("RemoteSystem", sysConfig)
    val registryActor = system.actorOf (Props (new RegistryService (actorConfig.addr, actorConfig.port)), name="registry")

    Runtime.getRuntime().addShutdownHook(new Thread {
      override def run = {
        system.actorSelection("/user/*") ! PoisonPill
      }
    })
  }
}
