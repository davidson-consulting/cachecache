package com.socialnet.utils.actor_request

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
import com.socialnet.utils.cache._

/**
  *  A request is an asychronous call to another actor
  *  It send a request, and wait for the answer within a given timeout
  *  A request is sent to the registry, to acquire an actoref to an actor that can answer to the request
  */
class Request [A : ClassTag] (registry : ActorSelection, cache : RequestCache, service : String, msg : Any, timeout : Int) {

  var future : Future[Response [A]] = null
  var startTimeReg : Long = 0
  var endTimeReg : Long = 0
  var startTime: Long = 0
  var endTime: Long = 0

  /**
    * Send the request to the registry, and then the actor
    */
  def start (): Request[A] = {
    implicit val ec  = scala.concurrent.ExecutionContext.global

    this.future = Future {
      this.startTime = System.nanoTime()
      if (this.cache == null) {
        this.askRegistry ()
      } else {
        this.cache.getRqtCache[A] (service, msg) match {
          case Some (t) => {
            Response[A] (ResponseCodes.Ok, None, Some (t))
          }
          case _ => {
            val v = this.askRegistry ();
            v match {
              case Ok (_, value) =>
                this.cache.insertRqtCache[A] (service, msg, value);
              case _ => {} // don't insert failing rqt
            }
            v
          }
        }
      }
    }

    this
  }

  /**
    * Assuming the request was sent with start (), this function awaits the result and returns it
    */
  def value: Response [A] = {
    if (this.future == null) this.start ()

    try {
      val result = Await.ready (this.future, Duration (this.timeout, SECONDS)).value.get
      this.endTime = System.nanoTime ()

      result match {
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

  def elapsedTime: (Double, Double) = {
    ((this.endTime - this.startTime) / 1.0e6, (this.endTimeReg - this.startTimeReg) / 1.0e6)
  }

  private def askRegistry (): Response [A] = {
    implicit val out : Timeout = Duration (this.timeout, SECONDS)
    this.startTimeReg = System.nanoTime ()
    val future = registry ? ServiceAccessReq (service)
    implicit val ec  = scala.concurrent.ExecutionContext.global

    try {
      val res = Await.ready (future, Duration (this.timeout, SECONDS)).value.get
      this.endTimeReg = System.nanoTime ()

      res match {
        case Success (ServiceAccessResp (Some (actor))) => {
          this.askService[A] (actor, this.msg)
        }
        case other => {
          Response [A] (ResponseCodes.NoService, Some ("No service available"), None)
        }
      }
    } catch {
      case e: TimeoutException =>
        Response [A] (ResponseCodes.Timeout, Some (e.toString), None)
      case e =>
        Response [A] (ResponseCodes.Unexpected, Some (e.toString), None)
    }
  }

  private def askService[A : ClassTag] (service : ActorRef, msg : Any): Response [A] = {
    implicit val out : Timeout = Duration (this.timeout, SECONDS)
    val future = service ? msg

    implicit val ec: scala.concurrent.ExecutionContext = scala.concurrent.ExecutionContext.global
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
