package com.socialnet.utils.registry_request

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
import com.socialnet.utils.response._

/**
  *  A request is an asychronous call to the registry
  *  It send a request, and wait for the answer within a given timeout
  */
class RegistryRequest [A : ClassTag] (registry : ActorSelection, msg : Any, timeout : Int) {

  var future : Future[Response [A]] = null

  /**
    * Send the request to the registry, and then the actor
    */
  def start (): RegistryRequest[A] = {
    implicit val ec  = scala.concurrent.ExecutionContext.global

    this.future = Future {
      this.askRegistry ()
    }

    this
  }

  /**
    * Assuming the request was sent with start (), this function awaits the result and returns it
    */
  def value: Response [A] = {
    if (this.future == null) this.start ()

    try {
      Await.ready (this.future, Duration (this.timeout, SECONDS)).value.get match {
        case Success (a : Response [A]) =>
          a
        case Failure (reason) =>
          Response [A] (ResponseCodes.Unexpected, Some (reason.toString), None)
      }
    } catch {
      case e: TimeoutException =>
        Response [A] (ResponseCodes.Timeout, Some (e.toString), None)
      case e =>
        Response [A] (ResponseCodes.Unexpected, Some (e.toString), None)
    }
  }

  private def askRegistry (): Response [A] = {
    implicit val out : Timeout = Duration (this.timeout, SECONDS)
    val future = registry ? msg
    implicit val ec  = scala.concurrent.ExecutionContext.global

    try {
      Await.ready (future, Duration (this.timeout, SECONDS)).value.get match {
        case Success (t : A) =>
          Response [A] (ResponseCodes.Ok, None, Some (t))
        case Success (x) =>
          Response [A] (ResponseCodes.WrongType, Some ("Wrong response type : " + x.getClass.getSimpleName), None)
        case Failure (e) => {
          Response [A] (ResponseCodes.Unexpected, Some (e.toString), None)
        }
      }
    } catch {
      case e: TimeoutException =>
        Response [A] (ResponseCodes.Timeout, Some (e.toString), None)
      case e =>
        Response [A] (ResponseCodes.Unexpected, Some (e.toString), None)
    }
  }
}
