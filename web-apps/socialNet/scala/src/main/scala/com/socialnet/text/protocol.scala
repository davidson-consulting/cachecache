package com.socialnet.text.protocol

import com.socialnet.utils.serializer._
import akka.actor.{ActorRef}
import com.socialnet.utils.request._
import com.socialnet.utils.actor_request._


case class TextRequest (
  content : String
) extends DefaultSerializer

case class TextResponse (
  content : String,
  users : Map[String, Int]
) extends DefaultSerializer


object TextService {
  val NAME = "TextService"

  def compute (rqtBuilder : RequestBuilder, content : String, timeout : Int = 15): Request[TextResponse] = {
    rqtBuilder.ask [TextResponse] (this.NAME, TextRequest (content), timeout = timeout)
  }


}
