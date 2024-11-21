package com.socialnet.users.service

import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill  }

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.utils.request._
import com.socialnet.utils.response._
import com.socialnet.utils.service._
import com.socialnet.utils.jwt_tokens._

import com.socialnet.users.database._
import com.socialnet.users.cache._
import com.socialnet.users.protocol._
import com.socialnet.users.data.users._

import org.mindrot.jbcrypt.BCrypt

class UserServiceActor (
  name : String,
  config: ServiceConfig,
  rqtCacheConfig : CacheConfig,
  cacheConfig : CacheConfig,
  dbConfig : DBConfig,
  registryConfig: RegistryConfig
)
    extends ServiceActor (UserService.NAME, registryConfig, rqtCacheConfig)
{

  var database : UserDatabase = null
  var cache : UserCache = null
  var userReg : UserRegistry = null

  override def postActivation (): Unit = {
    this.database = new UserDatabase (this.dbConfig)
    this.cache = new UserCache (this.cacheConfig)
    this.database.connect ()
    this.cache.connect ()

    val ttl = if (this.config.params.contains ("cache-TTL")) {
      this.config.params ("cache-TTL").asInstanceOf [Int]
    } else {
      this.cacheConfig.TTL
    }

    this.userReg = new UserRegistry (ttl, this.database, this.cache)
  }

  override def processMessage (msg: Any, replyTo: ActorRef): Unit = {
    msg match {

      // Log a user
      case LoginReq (login, pass) =>
        this.logUser (login, pass, replyTo)

      case UserIdsReq (logins) =>
        this.findUsers (logins, replyTo)

      case UserLoginsReq (ids) =>
        this.findUsersByIds (ids, replyTo)

      case UserExistsReq (id) =>
        this.checkUserExists (id, replyTo)

      case UserLoginReq (id) =>
        this.findUserLoginById (id, replyTo)

      case e =>
        log.error (s"Unmanaged message $e")
    }
  }

  /**
    * Check that the login and password match a user in database
    * Send a response to 'replyTo', with jwt token to use for future requests
    */
  def logUser (login: String, pass: String, replyTo: ActorRef): Unit = {
    this.userReg.findByLogin (login) match {
      case Some (User (id,log,hash)) =>
        if (BCrypt.checkpw (pass, hash)) {
          replyTo ! LoginResp (id, login, true, Jwt (Map ("login"-> login, "id"-> id, "issued"-> s"UserService@${this.name}")))
        } else {
          replyTo ! LoginResp (0, login, false, Jwt (Map ()))
        }
      case None =>
        replyTo ! LoginResp (0, login, false, Jwt (Map ()))
    }
  }

  /**
    * Find the ids from a list of user logins
    */
  def findUsers (logins: List[String], replyTo: ActorRef): Unit = {
    val users = this.userReg.findByLogins (logins)
    var result: Map[String, Int] = Map ()
    for (u <- users) {
      result = result + (u.login-> u.id)
    }

    replyTo ! UserIdsResp (result)
  }

  /**
    * Find the logins from a list of user ids
    */
  def findUsersByIds (ids: List[Int], replyTo: ActorRef): Unit = {
    val users = this.userReg.findByIds (ids)
    var result: Map[String, Int] = Map ()
    for (u <- users) {
      result = result + (u.login-> u.id)
    }

    replyTo ! UserIdsResp (result)
  }

  /**
    * Check if a user with a given id exists
    */
  def checkUserExists (id: Int, replyTo: ActorRef): Unit = {
    val user = this.userReg.findById (id) match {
      case Some (_) =>
        replyTo ! true
      case _ =>
        replyTo ! false
    }
  }

  /**
    * Get user login from id
    */
  def findUserLoginById (id: Int, replyTo: ActorRef): Unit = {
    val user = this.userReg.findById (id) match {
      case Some (u) =>
        replyTo ! UserLoginResp (u.id, u.login)
      case _ =>
        replyTo ! UserExistsResp (false)
    }
  }

}
