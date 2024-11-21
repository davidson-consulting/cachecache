package com.socialnet.timeline.protocol

import com.socialnet.utils.serializer._
import com.socialnet.utils.request._
import com.socialnet.utils.actor_request._

case class UpdateTimelineReq (
  postId: Int,
  userId: Int,
  userTags: List[Int]
) extends DefaultSerializer

case class HomeTimelineReq (
  userId: Int,
  page: Int,
  nb: Int
) extends DefaultSerializer

case class HomeTimelineSizeReq (
  userId: Int
) extends DefaultSerializer

case class UserTimelineReq (
  userId: Int,
  page: Int,
  nb: Int
) extends DefaultSerializer

case class UserTimelineSizeReq (
  userId: Int
) extends DefaultSerializer



object TimelineService {
  val NAME = "TimelineService"

  def update (rqtBuilder : RequestBuilder, postId : Int, userId : Int, userMentions : List[Int], timeout : Int = 15): Request[String] = {
    rqtBuilder.ask[String] (this.NAME, UpdateTimelineReq (postId, userId, userMentions), timeout = timeout)
  }

  def getHomeTimeline (rqtBuilder : RequestBuilder, userId : Int, page : Int, nb : Int, timeout : Int = 15): Request[List[Int]] = {
    rqtBuilder.ask[List[Int]] (this.NAME, HomeTimelineReq (userId, page, nb), timeout = timeout)
  }

  def getHomeTimelineSize (rqtBuilder : RequestBuilder, userId : Int, timeout : Int = 15): Request[List[Int]] = {
    rqtBuilder.ask[List[Int]] (this.NAME, HomeTimelineSizeReq (userId), timeout = timeout)
  }

  def getUserTimeline (rqtBuilder : RequestBuilder, userId : Int, page : Int, nb : Int, timeout : Int = 15): Request[List[Int]] = {
    rqtBuilder.ask[List[Int]] (this.NAME, UserTimelineReq (userId, page, nb), timeout = timeout)
  }

  def getUserTimelineSize (rqtBuilder : RequestBuilder, userId : Int, timeout : Int = 15): Request[List[Int]] = {
    rqtBuilder.ask[List[Int]] (this.NAME, UserTimelineSizeReq (userId), timeout = timeout)
  }
}
