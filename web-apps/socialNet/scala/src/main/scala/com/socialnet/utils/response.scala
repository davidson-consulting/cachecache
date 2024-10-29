package com.socialnet.utils.response

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

object ResponseCodes extends Enumeration {
  type ResponseCode = Value;

  val Ok,WrongType,Timeout,NoService, Unexpected = Value;
}

case class Response [A : ClassTag] (val code : ResponseCodes.ResponseCode, val error : Option[String], val content : Option[A])

object Ok {
  def unapply[A : ClassTag] (resp : Response[A]): Option[(ResponseCodes.ResponseCode, A)] = {
    resp.content match {
      case Some (x) => Some ((resp.code, x))
      case _ => None
    }
  }
}

object Fail {
  def unapply[A : ClassTag] (resp : Response[A]): Option[(ResponseCodes.ResponseCode, String)] = {
    resp.content match {
      case Some (_) => None
      case _ => {
        resp.error match {
          case Some (x) => Some ((resp.code, x))
          case _ => Some ((resp.code, "Empty response"))
        }
      }
    }
  }
}

object ResponseGetter {
  def getOr[A : ClassTag] (resp : Response[A], or : A): A = {
    resp match {
      case Ok (_, a : A) => a
      case _ =>
        or
    }
  }
}
