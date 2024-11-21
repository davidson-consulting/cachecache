package com.socialnet.compose.service

import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill  }

import scala.collection.mutable.ListBuffer

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.utils.request._
import com.socialnet.text.protocol._
import com.socialnet.compose.protocol._
import com.socialnet.users.protocol._
import com.socialnet.timeline.protocol.{HomeTimelineReq => TIMEHomeTimelineReq, UserTimelineReq => TIMEUserTimelineReq, TimelineService}
import com.socialnet.post.protocol._
import com.socialnet.social_graph.protocol.{SubscriptionReq => SOCIALSubscriptionReq, FollowersReq => SOCIALFollowersReq, SocialGraphService}

import com.socialnet.utils.service._
import com.socialnet.utils.actor_request._
import com.socialnet.utils.response._
import com.socialnet.utils.jwt_tokens._

import java.util.regex.{Matcher, Pattern}

class ComposeServiceActor (
  name : String,
  config: ServiceConfig,
  rqtCacheConfig : CacheConfig,
  cacheConfig : CacheConfig,
  registryConfig: RegistryConfig
)
    extends ServiceActor (ComposeService.NAME, registryConfig, rqtCacheConfig)
{

  override def postActivation() : Unit = {
    println (s"COMPOSE : ${this.rqtCache} ${rqtCacheConfig} ${config.rqt_cache}")
  }

  override def processMessage (msg : Any, replyTo : ActorRef): Unit = {
    msg match {
      case PostSubmitReq (userId, content, token) =>
        this.submitNewPost (userId, content, token, replyTo)
      case HomeTimelineReq (userId, page, nb) =>
        this.retreiveHomeTimeline (userId, page, nb, replyTo)
      case UserTimelineReq (userId, page, nb) =>
        this.retreiveUserTimeline (userId, page, nb, replyTo)
      case SubscriptionReq (userId, page, nb) =>
        this.retreiveSubscriptions (userId, page, nb, replyTo)
      case FollowersReq (userId, page, nb) =>
        this.retreiveFollowers (userId, page, nb, replyTo)
      case e =>
        log.error (s"Unmanaged message $e")
    }
  }

  /**
    * Submission of a new post
    */
  def submitNewPost (userId: Int, content: String, token: String, replyTo: ActorRef): Unit = {
    token match {
      case Jwt (claims) => {
        if (claims.contains ("id") && claims ("id") == userId) {
          TextService.compute (this.rqtBuilder, content).value match {
            case Ok (_, TextResponse (newText, userTags)) => { // retreive the parsed content
              val userLogin = UserService.getLogin (this.rqtBuilder, userId).value match {
                case Ok (_, id) => { id.login }
                case _ => { "??" }
              }

              PostStorageService.store (this.rqtBuilder, userId, userLogin, newText, userTags.values.toList, token).value match {
                case Ok (_,_) => { // storing is successful
                  replyTo ! true
                }
                case _ =>
                  replyTo ! false
              }
            }
            case _ =>
              replyTo ! false
          }
        } else {
          replyTo ! false
        }
      }
      case _ =>
        replyTo ! false
    }
  }

  /**
    * Retreive the posts composing the home timeline of 'userId' (pageable)
    */
  def retreiveHomeTimeline (userId: Int, page: Int, nb: Int, replyTo: ActorRef): Unit = {
    TimelineService.getHomeTimeline (this.rqtBuilder, userId, page, nb).value match {
      case Ok (_, list) => {
        PostStorageService.read (this.rqtBuilder, list).value match {
          case Ok (_, lst) => {
            replyTo ! TimelineResp (Some (lst))
          }
          case _ => {
            replyTo ! TimelineResp (None)
          }
        }
      }
      case _ => {
        replyTo ! TimelineResp (None)
      }
    }
  }

  /**
    * Retreive the posts composing the user timeline of 'userId' (pageable)
    */
  def retreiveUserTimeline (userId: Int, page: Int, nb: Int, replyTo: ActorRef): Unit = {
    TimelineService.getUserTimeline (this.rqtBuilder, userId, page, nb).value match {
      case Ok (_, list) => {
        PostStorageService.read (this.rqtBuilder, list).value match {
          case Ok (_, lst) => {
            replyTo ! TimelineResp (Some (lst))
          }
          case _ => {
            replyTo ! TimelineResp (None)
          }
        }
      }
      case _ => {
        replyTo ! TimelineResp (None)
      }
    }
  }

  /**
    * Retreive the list of users to whom the user 'userId' is subscribed (pageable)
    */
  def retreiveSubscriptions (userId: Int, page: Int, nb: Int, replyTo: ActorRef): Unit = {
    SocialGraphService.getSubscriptions (this.rqtBuilder, userId, page, nb).value match {
      case Ok (_, lst) => {
        UserService.getLogins (this.rqtBuilder, lst).value match {
          case Ok (_, UserIdsResp (users)) => {
            var result = ListBuffer[UserContent]()
            for (u <- users) {
              result += UserContent (u._2, u._1)
            }

            replyTo ! FollowOrSubsResp (Some (result.toList))
          }
          case _ => {
            replyTo ! FollowOrSubsResp (None)
          }
        }
      }
      case _ => {
        replyTo ! FollowOrSubsResp (None)
      }
    }
  }

  /**
    * Retreive the list of users subscribed to the user 'userId' (pageable)
    */
  def retreiveFollowers (userId: Int, page: Int, nb: Int, replyTo: ActorRef): Unit = {
    SocialGraphService.getFollowers (this.rqtBuilder, userId, page, nb).value match {
      case Ok (_, list) => {
        UserService.getLogins (this.rqtBuilder, list).value match {
          case Ok (_, UserIdsResp (users)) => {
            var result = ListBuffer[UserContent]()
            for (u <- users) {
              result += UserContent (u._2, u._1)
            }

            replyTo ! FollowOrSubsResp (Some (result.toList))
          }
          case _ => {
            replyTo ! FollowOrSubsResp (None)
          }
        }
      }
      case _ => {
        replyTo ! FollowOrSubsResp (None)
      }
    }
  }

}
