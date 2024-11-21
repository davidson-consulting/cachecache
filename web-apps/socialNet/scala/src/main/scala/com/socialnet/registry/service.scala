package com.socialnet.registry.service

import java.io.{File}
import scala.Array
import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill, Terminated  }
import com.typesafe.config.ConfigFactory

import akka.util.ByteString
import java.net.InetSocketAddress
import java.io._
import java.util.{Map => JMap, List => JArray}
import org.yaml.snakeyaml.Yaml
import scala.jdk.CollectionConverters._

import com.socialnet.master._
import com.socialnet.registry._
import com.socialnet.registry.protocol._
import com.socialnet.utils.request._
import com.socialnet.utils.jwt_tokens._

import java.util.{Date, UUID}

/**
  * A registry is a special service storing information about all the services running on a cluster
  * It uses a load balancer to redirect requests to the different services
  */
class RegistryService (addr : String, port : Int) extends Actor with ActorLogging {

  var actors : Map[String, List[ActorRef]] = Map ()
  var actorIndexes : Map[String, Int] = Map ()

  override def preStart () : Unit = {
    log.info (s"Registry is up on : $addr:$port")
  }

  /**
    * Protocol
    */
  def receive : Receive = {
    case RegisterService (kind, token, actor) => // new service
      this.registerActor (kind, token, actor, sender ())
    case UnregisterService (kind, token, actor) => // end of service
      this.unregisterService (kind, token, actor, sender ())
    case ServiceAccessReq (kind) => // request access to a service
      this.manageServiceAccess (kind, sender ())
    case x =>
      log.error (s"?? $x")
  }

  /**
    * On kill, inform all registered service that registry is dead
    */
  override def postStop () = {
    for (actor <- this.actors) {
      for (act <- actor._2) {
        act ! RegistryDied (Jwt (Map ("registry"-> "registry")))
      }
    }
  }

  def generateUniqId (): String = {
    UUID.randomUUID.toString.hashCode.toHexString
  }

  /**
    * An new actor was launched, we need to store it
    */
  def registerActor (kind: String, token: String, actor : ActorRef, replyTo : ActorRef) = {
    log.info (s"Service wants to register itself ${kind}")

    var success = false
    token match {
      case Jwt (claims) =>
        if (claims.contains ("service") && claims ("service") == kind) {
          success = true
          replyTo ! RegisterResponse (this.generateUniqId (), Jwt (Map ("registry"-> "registry")), "Hello")

          if (this.actors.contains (kind)) {
            this.actors += (kind-> (this.actors (kind) ++ List (actor)))
          } else {
            this.actors += (kind-> (List (actor)))
            this.actorIndexes += (kind -> 0)
          }
        }
    }

    if (!success) {
      log.error (s"Service $kind from $actor is not official, ignoring !")
    }
  }

  /**
    * A service was ended, we need to store that information to avoid sending requests to it
    */
  def unregisterService (kind: String, token: String, actor: ActorRef, replyTo : ActorRef) = {
    log.info (s"Service wants to unregister itself ${kind}")
    var success = false

    token match {
      case Jwt (claims) =>
        if (claims.contains ("service") && claims ("service") == kind) {
          success = true
          if (this.actors.contains (kind)) {
            this.actors += (kind-> this.actors (kind).filter (_ != actor))
            if (this.actors (kind).size == 0) {
              this.actors -= kind;
              this.actorIndexes -= kind;
            } else {
              this.actorIndexes += (kind-> 0);
            }
          }

          replyTo ! UnregisterResponse ("ACK")
        }
    }

    if (!success) {
      log.error (s"Service $kind from $actor is not official, ignoring !")
    }
  }

  /**
    * A service is requesting access to another service
    */
  def manageServiceAccess (kind : String, replyTo : ActorRef) = {
    log.info (s"Request for service of kind $kind for $replyTo")

    if (this.actors.contains (kind)) {
      // Create a load balance between service, making a circle selection (taking the first one and putting it at the end)
      val index = this.actorIndexes (kind)
      val actor = this.actors (kind) (index);
      replyTo ! ServiceAccessResp (Some (actor))

      this.actorIndexes += (kind -> ((this.actorIndexes (kind) + 1) % this.actors (kind).size))
    } else {
      replyTo ! ServiceAccessResp (None)
    }
  }



}
