package com.socialnet.social_graph.service

import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill  }

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.users.protocol._

import com.socialnet.utils.request._
import com.socialnet.utils.service._
import com.socialnet.utils.response._
import com.socialnet.utils.jwt_tokens._

import com.socialnet.social_graph.protocol._
import com.socialnet.social_graph.database._
import com.socialnet.social_graph.cache._
import com.socialnet.social_graph.data.sub._


class SocialGraphServiceActor (
  name : String,
  config: ServiceConfig,
  rqtCacheConfig : CacheConfig,
  cacheConfig : CacheConfig,
  dbConfig : DBConfig,
  registryConfig: RegistryConfig
)
    extends ServiceActor (SocialGraphService.NAME, registryConfig, rqtCacheConfig)
{

  var database: SocialGraphDatabase = null
  var cache : SocialGraphCache = null

  var subReg : SubRegistry = null

  override def postActivation (): Unit = {
    this.database = new SocialGraphDatabase (this.dbConfig)
    this.cache = new SocialGraphCache (this.cacheConfig)
    this.database.connect ()
    this.cache.connect ()

    val ttl = if (this.config.params.contains ("cache-TTL")) {
      this.config.params ("cache-TTL").asInstanceOf [Int]
    } else {
      cacheConfig.TTL
    }

    this.subReg = new SubRegistry (ttl, this.database, this.cache)
  }

  override def processMessage (msg: Any, replyTo: ActorRef): Unit = {
    msg match {
      case FollowReq (userId, toWhom, token) =>
        this.treatFollow (userId, toWhom, token, replyTo)

      case UnFollowReq (userId, toWhom, token) =>
        this.treatUnFollow (userId, toWhom, token, replyTo)

      case IsFollowingReq (userId, toWhom) =>
        this.treatCheckIsFollowing (userId, toWhom, replyTo)

      case FollowersReq (userId, page, nb) =>
        this.treatGetFollowers (userId, page, nb, replyTo)

      case SubscriptionReq (userId, page, nb) =>
        this.treatGetSubs (userId, page, nb, replyTo)

      case NbSubscriptions (userId) =>
        this.treatNbSubs (userId, replyTo)

      case NbFollowers (userId) =>
        this.treatNbFollowers (userId, replyTo)

      case e =>
        log.error (s"Unmanaged message $e")
    }
  }

  /**
    * Treat a following request
    * @params:
    *    - userId: the user performing the request
    *    - toWhom: the user that will be followed
    *    - token: the jwt token to check user permissions
    *    - replyTo: the service performing the request
    */
  def treatFollow (userId: Int, toWhom: Int, token: String, replyTo: ActorRef): Unit = {
    if (userId == toWhom) {
      replyTo ! false // you cannot follow yourself
      return
    }

    token match {
      case Jwt (claims) => {
        if (claims.contains ("id") && claims ("id") == userId) { // User correctly authenticated
          UserService.exists (this.rqtBuilder, toWhom).value match {
            case Ok (_, true) => { // toWhom is a user (no need to check for userId, we assume that if it has a jwt token the user exists)
              this.subReg.insertIfNotExist (Sub (userId, toWhom))
              replyTo ! true // success
            }
            case _ => {
              replyTo ! false // failure, toWhom does not exist, or request error
            }
          }
        } else {
          replyTo ! false // failure, user not authenticated or not the correct user
        }
      }
      case _ =>
        replyTo ! false // failure, user not authenticated
    }
  }

  /**
    * Treat an unfollowing request
    * @params:
    *    - userId: the user performing the request
    *    - toWhom: the user that was followed
    *    - token: the jwt token to check user permissions
    *    - replyTo: the service performing the request
    */
  def treatUnFollow (userId: Int, toWhom: Int, token: String, replyTo: ActorRef): Unit = {
    if (userId == toWhom) {
      replyTo ! false // you cannot follow yourself
      return
    }

    token match {
      case Jwt (claims) => {
        if (claims.contains ("id") && claims ("id") == userId) { // User correctly authenticated
          this.subReg.delete (Sub (userId, toWhom)) // no need to check if toWhom exists, it won't be present in the table anyway if not
          replyTo ! true // success
        } else {
          replyTo ! false // failure, user not authenticated or not the correct user
        }
      }
      case _ =>
        replyTo ! false // failure, user not authenticated
    }
  }

  /**
    * Treat a check of following
    * @params:
    *    - userId: the user following
    *    - toWhom: the user being followed
    *    - replyTo: the actor performing the req
    */
  def treatCheckIsFollowing (userId: Int, toWhom: Int, replyTo: ActorRef): Unit = {
    if (userId == toWhom) {
      replyTo ! false
    }

    this.subReg.find (Sub (userId, toWhom)) match {
      case Some (_) =>
        replyTo ! true
      case _ =>
        replyTo ! false
    }
  }

  /**
    * Treat a get followers request
    * @params:
    *     - userId: the user begin followed
    */
  def treatGetFollowers (userId: Int, page: Int, nb: Int, replyTo: ActorRef): Unit = {
    val lst = this.subReg.findByToWhom (userId, page, nb).map (_.userId)
    replyTo ! lst
  }

  /**
    * Treat a get subscriptions request
    * @params:
    *     - userId: the user begin followed
    */
  def treatGetSubs (userId: Int, page: Int, nb: Int, replyTo: ActorRef): Unit = {
    val lst = this.subReg.findByUserId (userId, page, nb).map (_.toWhom)
    replyTo ! lst
  }

  /**
    * Count the number of users subscribed to 'userId'
    */
  def treatNbFollowers (userId: Int, replyTo: ActorRef): Unit = {
    val nb = this.subReg.countNbFollowers (userId)
    replyTo ! nb
  }

  /**
    * Count the number of subscriptions of 'userId'
    */
  def treatNbSubs (userId: Int, replyTo: ActorRef): Unit = {
    val nb =  this.subReg.countNbSubscriptions (userId)
    replyTo ! nb
  }

}
