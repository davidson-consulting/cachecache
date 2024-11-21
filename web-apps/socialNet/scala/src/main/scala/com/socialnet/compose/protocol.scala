package com.socialnet.compose.protocol

import com.socialnet.utils.serializer._
import com.socialnet.utils.request._
import com.socialnet.utils.actor_request._
import com.socialnet.post.protocol._
import akka.actor.{ActorRef}


case class UserContent (
  userId: Int,
  login: String
) extends DefaultSerializer

case class PostSubmitReq (
  userId: Int,
  content: String,
  token: String
) extends DefaultSerializer

case class HomeTimelineReq (
  userId: Int,
  page: Int,
  nb: Int
) extends DefaultSerializer

case class UserTimelineReq (
  userId: Int,
  page: Int,
  nb: Int
) extends DefaultSerializer

case class TimelineResp (
  lst: Option[List[PostContent]]
) extends DefaultSerializer

case class SubscriptionReq (
  userId: Int,
  page: Int,
  nb: Int
) extends DefaultSerializer

case class FollowersReq (
  userId: Int,
  page: Int,
  nb: Int
) extends DefaultSerializer


case class FollowOrSubsResp (
  lst: Option[List[UserContent]]
) extends DefaultSerializer

object ComposeService {

  val NAME = "ComposeService"

  def submitPost (rqtBuilder : RequestBuilder, userId : Int, text : String, token : String, timeout : Int = 15): Request[Boolean] = {
    rqtBuilder.ask [Boolean] (this.NAME, PostSubmitReq (userId, text, token), timeout = timeout)
  }

  def getHomeTimeline (rqtBuilder : RequestBuilder, userId : Int, page : Int, nb : Int, timeout : Int = 15): Request[TimelineResp] = {
    rqtBuilder.ask [TimelineResp] (this.NAME, HomeTimelineReq (userId, page, nb), timeout = timeout)
  }

  def getUserTimeline (rqtBuilder : RequestBuilder, userId : Int, page : Int, nb : Int, timeout : Int = 15): Request[TimelineResp] = {
    rqtBuilder.ask [TimelineResp] (this.NAME, UserTimelineReq (userId, page, nb), timeout = timeout)
  }

  def getSubscriptions (rqtBuilder : RequestBuilder, userId : Int, page : Int, nb : Int, timeout : Int = 15): Request[FollowOrSubsResp] = {
    rqtBuilder.ask [FollowOrSubsResp] (this.NAME, SubscriptionReq (userId, page, nb), timeout = timeout)
  }

  def getFollowers (rqtBuilder : RequestBuilder, userId : Int, page : Int, nb : Int, timeout : Int = 15): Request[FollowOrSubsResp] = {
    rqtBuilder.ask [FollowOrSubsResp] (this.NAME, FollowersReq (userId, page, nb), timeout = timeout)
  }

}
