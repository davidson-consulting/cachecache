package com.socialnet.users.protocol

import com.socialnet.utils.serializer._
import com.socialnet.utils.request._
import com.socialnet.utils.actor_request._

case class UserIdsReq (
  logins: List[String]
) extends DefaultSerializer

case class UserLoginsReq (
  ids: List[Int]
) extends DefaultSerializer

case class UserIdsResp (
  ids: Map [String, Int]
) extends DefaultSerializer

case class LoginReq (
  login: String,
  password: String
) extends DefaultSerializer

case class LoginResp (
  id: Int,
  login: String,
  success: Boolean,
  token: String
) extends DefaultSerializer

case class UserLoginReq (
  id: Int
) extends DefaultSerializer


case class UserLoginResp (
  id: Int,
  login: String
) extends DefaultSerializer

case class UserExistsReq (
  id: Int
) extends DefaultSerializer

case class UserExistsResp (
  yes: Boolean
) extends DefaultSerializer


object UserService {
  val NAME = "UserService"

  def logUser (rqtBuilder : RequestBuilder, login : String, password : String, timeout : Int = 15): Request[LoginResp] = {
    rqtBuilder.ask[LoginResp] (this.NAME, LoginReq (login, password), timeout = timeout)
  }

  def getLogin (rqtBuilder : RequestBuilder, userId : Int, timeout : Int = 15): Request[UserLoginResp] = {
    rqtBuilder.ask[UserLoginResp] (this.NAME, UserLoginReq (userId), timeout = timeout)
  }

  def getLogins (rqtBuilder : RequestBuilder, userId : List[Int], timeout : Int = 15): Request[UserIdsResp] = {
    rqtBuilder.ask[UserIdsResp] (this.NAME, UserLoginsReq (userId), timeout = timeout)
  }

  def getIds (rqtBuilder : RequestBuilder, logins : List[String], timeout : Int = 15): Request[UserIdsResp] = {
    rqtBuilder.ask[UserIdsResp] (this.NAME, UserIdsReq (logins), timeout = timeout)
  }

  def exists (rqtBuilder : RequestBuilder, userId : Int, timeout : Int = 15): Request[Boolean] = {
    rqtBuilder.ask[Boolean] (this.NAME, UserExistsReq (userId), timeout = timeout)
}

}
