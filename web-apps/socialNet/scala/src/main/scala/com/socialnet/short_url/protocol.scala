package com.socialnet.short_url.protocol

import com.socialnet.utils.serializer._
import com.socialnet.utils.request._
import com.socialnet.utils.actor_request._

case class LongToShortReq (
  long: String
) extends DefaultSerializer

case class LongToShortResp (
  short: String
) extends DefaultSerializer

case class ShortToLongReq (
  short: String
) extends DefaultSerializer

case class ShortToLongResp (
  long: Option[String]
) extends DefaultSerializer



object ShortUrlService {
  val NAME = "ShortUrlService"

  def shorten (rqtBuilder : RequestBuilder, url : String, timeout : Int = 15): Request[LongToShortResp] = {
    rqtBuilder.ask[LongToShortResp] (this.NAME, LongToShortReq (url), timeout = timeout)
  }


}
