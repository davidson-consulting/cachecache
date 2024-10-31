package com.socialnet.social_graph.protocol

import com.socialnet.utils.serializer._
import com.socialnet.utils.request._
import com.socialnet.utils.actor_request._


case class FollowReq (
  userId: Int,
  toWhom: Int,
  token: String
) extends DefaultSerializer

case class UnFollowReq (
  userId: Int,
  toWhom: Int,
  token: String
) extends DefaultSerializer

case class IsFollowingReq (
  userId: Int,
  who: Int
) extends DefaultSerializer

case class FollowersReq (
  userId: Int,
  page: Int,
  nb: Int
) extends DefaultSerializer

case class SubscriptionReq (
  userId: Int,
  page: Int,
  nb: Int
) extends DefaultSerializer

case class NbFollowers (
  userId: Int
) extends DefaultSerializer

case class NbSubscriptions (
  userId: Int
) extends DefaultSerializer



object SocialGraphService {
  val NAME = "SocialGraphService"

  def getSubscriptions (rqtBuilder : RequestBuilder, userId : Int, page : Int, nb : Int, timeout : Int = 15): Request[List[Int]] = {
    rqtBuilder.ask [List[Int]] (this.NAME, SubscriptionReq (userId, page, nb), timeout = timeout)
  }

  def getFollowers (rqtBuilder : RequestBuilder, userId : Int, page : Int, nb : Int, timeout : Int = 15): Request[List[Int]] = {
    rqtBuilder.ask [List[Int]] (this.NAME, FollowersReq (userId, page, nb), timeout = timeout)
  }

  def getNbSubscriptions (rqtBuilder : RequestBuilder, userId : Int, timeout : Int = 15): Request[Int] = {
    rqtBuilder.ask [Int] (this.NAME, NbSubscriptions (userId), timeout = timeout)
  }

  def getNbFollowers (rqtBuilder : RequestBuilder, userId : Int, timeout : Int = 15): Request[Int] = {
    rqtBuilder.ask [Int] (this.NAME, NbFollowers (userId), timeout = timeout)
  }

  def follow (rqtBuilder : RequestBuilder, userId : Int, toWhom : Int, token : String, timeout : Int = 15): Request[Boolean] = {
    rqtBuilder.ask [Boolean] (this.NAME, FollowReq (userId, toWhom, token), timeout = timeout)
  }

  def unfollow (rqtBuilder : RequestBuilder, userId : Int, toWhom : Int, token : String, timeout : Int = 15): Request[Boolean] = {
    rqtBuilder.ask [Boolean] (this.NAME, UnFollowReq (userId, toWhom, token), timeout = timeout)
  }

  def isFollowing (rqtBuilder : RequestBuilder, userId : Int, toWhom : Int, timeout : Int = 15): Request[Boolean] = {
    rqtBuilder.ask [Boolean] (this.NAME, IsFollowingReq (userId, toWhom), timeout = timeout)
  }

}
