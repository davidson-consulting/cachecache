package com.socialnet.utils.service

import akka.actor._
import com.typesafe.config.ConfigFactory
import scala.reflect.ClassTag

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.utils.response._
import com.socialnet.utils.cache._
import com.socialnet.utils.actor_request._
import com.socialnet.utils.request._
import com.socialnet.utils.jwt_tokens._

abstract class ServiceActor (kind : String, registryConfig : RegistryConfig, cacheConfig : CacheConfig) extends Actor with ActorLogging {

  var registry : ActorSelection = null
  var rqtBuilder : RequestBuilder = null
  var rqtCache : RequestCache = null
  var id: String = null

  override def preStart (): Unit = {
    this.registry = context.actorSelection (s"akka://RemoteSystem@${registryConfig.addr}:${registryConfig.port}/user/registry")
    val token = Jwt (Map ("service"-> this.kind))

    this.rqtCache = new RequestCache (kind, cacheConfig.TTL, cacheConfig.addr, cacheConfig.port);
    this.rqtCache.connect ()

    RequestBuilder.askRegistry [RegisterResponse] (this.registry, RegisterService (kind, token, context.self), timeout = 10).value match {
      case Ok (code, RegisterResponse (id, token, msg)) =>
        var success = false
        token match {
          case Jwt (claims) =>
            if (claims.contains ("registry") && claims ("registry").toString == "registry") {
              log.info (s"$kind is registered : $id, $msg")

              this.id = id
              context.become (active)
              this.postActivation ()
              success = true
            }
          case _ => {}
        }

        if (!success) {
          log.error (s"Registry is an usurper !! Aborting.")
          context.stop (self)
        }

      case Fail (code, msg) =>
        log.error (s"Fail to register service $kind : $code, $msg")
        context.stop (self)
      case _ =>
        log.error (s"Fail to register service $kind : for unknown reasons")
        context.stop (self)
    }

    this.rqtBuilder = new RequestBuilder (this.registry, this.rqtCache)
  }

  def receive: Receive = {
    case e =>
      println (s"$e")
      log.error (s"Service not active yet ignoring message : $e")
  }

  def active: Receive = {
    case RegistryDied (jwt) =>
      jwt match {
        case Jwt (claims) => {
          if (claims.contains ("registry") && claims ("registry") == "registry") {
            log.info ("Registry is dead aborting !")
            context.stop (self)
          } else {
            log.error ("Usurper registry tried to kill me !")
          }
        }
        case _ => {
          log.error ("Usurper registry tried to kill me !")
        }
      }

    case x =>
      this.processMessage (x, sender ())

    case _ =>

  }

  override def postStop () = {
    val token = Jwt (Map ("service"-> this.kind))

    RequestBuilder.askRegistry [UnregisterResponse] (this.registry, UnregisterService (kind, token, context.self), timeout = 10).value match {
      case Ok (code, UnregisterResponse (msg)) =>
        log.info (s"$kind is unregistered : $msg")
      case Fail (code, msg) =>
        log.error (s"Fail to unregister service $kind : $code, $msg")
      case _ =>
        log.error (s"Fail to unregister service $kind : for unknown reasons")
    }

    this.postKill ()
  }

  def postActivation (): Unit;
  def processMessage (msg : Any, replyTo : ActorRef): Unit;
  def postKill (): Unit = {}

  def request[T : ClassTag] (serviceName : String, msg : Any, timeout : Int = 15): Request[T] = {
    this.rqtBuilder.ask[T] (serviceName, msg, timeout = timeout)
  }

}
