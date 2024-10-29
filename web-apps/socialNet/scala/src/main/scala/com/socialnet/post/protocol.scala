package com.socialnet.post.protocol

import com.socialnet.utils.serializer._
import com.socialnet.utils.request._
import com.socialnet.utils.actor_request._

case class StorePostReq (
  userId: Int,
  userLogin : String,
  text: String,
  userMentions: List[Int],
  token: String // jwt token to identify logged user
) extends DefaultSerializer

case class ReadPostReq (
  postId: Int
) extends DefaultSerializer

case class ReadMultiplePostsReq (
  ids: List[Int]
) extends DefaultSerializer

case class PostContent (
  postId: Int,
  userId: Int,
  userLogin : String,
  text: String,
  userMentions: List[Int]
) extends DefaultSerializer



object PostStorageService {
  val NAME = "PostStorageService"

  def store (rqtBuilder : RequestBuilder, userId : Int, userLogin : String, text : String, userTags : List[Int], token : String, timeout : Int = 15): Request[Boolean] = {
    rqtBuilder.ask [Boolean] (this.NAME, StorePostReq (userId, userLogin, text, userTags, token), timeout = timeout)
  }

  def readOne (rqtBuilder : RequestBuilder, post : Int, timeout : Int = 15): Request[PostContent] = {
    rqtBuilder.ask [PostContent] (this.NAME, ReadPostReq (post), timeout = timeout)
  }

  def read (rqtBuilder : RequestBuilder, posts : List[Int], timeout : Int = 15): Request[List[PostContent]] = {
    rqtBuilder.ask [List[PostContent]] (this.NAME, ReadMultiplePostsReq (posts), timeout = timeout)
  }

}
