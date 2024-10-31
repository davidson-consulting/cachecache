package com.socialnet.utils.request

import com.socialnet.registry.protocol._

import akka.actor._
import akka.pattern.ask
import akka.util.Timeout
import scala.util.{Success, Failure}
import scala.reflect.ClassTag

import akka.actor.typed.scaladsl.AskPattern._
import scala.concurrent.duration._
import scala.concurrent.{Await, Future}

import java.util.concurrent.{TimeoutException}
import com.socialnet.utils.actor_request._
import com.socialnet.utils.cache._
import com.socialnet.utils.registry_request._
import com.socialnet.utils.response._

object RequestBuilder {

  /**
    * Create a request to another microservice
    */
  def ask[A : ClassTag] (registry : ActorSelection, cache : RequestCache, service : String, msg : Any, timeout : Int = 15): Request[A] = {
    new Request (registry, cache, service, msg, timeout)
  }

  /**
    * Create a request directly to the registry
    */
  def askRegistry [A : ClassTag] (registry : ActorSelection, msg : Any, timeout : Int = 15): RegistryRequest[A] = {
    new RegistryRequest (registry, msg, timeout)
  }

}


class RequestBuilder (registry : ActorSelection, cache : RequestCache) {

  def ask[A: ClassTag] (service : String, msg : Any, timeout : Int = 15): Request [A] = {
    RequestBuilder.ask[A] (this.registry, this.cache, service, msg, timeout)
  }

}
