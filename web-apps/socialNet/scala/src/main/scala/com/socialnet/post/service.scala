package com.socialnet.post.service

import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill  }

import scala.collection.mutable.ListBuffer

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.utils.request._

import com.socialnet.timeline.protocol._
import com.socialnet.post.protocol._
import com.socialnet.post.data.tags._
import com.socialnet.post.data.post._
import com.socialnet.post.database._
import com.socialnet.post.cache._

import com.socialnet.utils.service._
import com.socialnet.utils.actor_request._
import com.socialnet.utils.response._
import com.socialnet.utils.jwt_tokens._

import java.util.regex.{Matcher, Pattern}

class PostStorageServiceActor (
  name : String,
  config: ServiceConfig,
  rqtCacheConfig : CacheConfig,
  cacheConfig : CacheConfig,
  dbConfig : DBConfig,
  registryConfig: RegistryConfig
)
    extends ServiceActor (PostStorageService.NAME, registryConfig, rqtCacheConfig)
{

  var database : PostDatabase = null
  var cache : PostCache = null

  var postReg : PostRegistry = null;
  var userTagReg : UserTagsRegistry = null;

  override def postActivation() : Unit = {
    this.database = new PostDatabase (this.dbConfig)
    this.cache = new PostCache (this.cacheConfig)
    this.database.connect ()
    this.cache.connect ()

    val ttl = if (this.config.params.contains ("cache-TTL")) {
      this.config.params ("cache-TTL").asInstanceOf [Int]
    } else {
      cacheConfig.TTL
    }

    this.postReg = new PostRegistry (ttl, this.database, this.cache)
    this.userTagReg = new UserTagsRegistry (ttl, this.database, this.cache)
  }

  override def processMessage (msg : Any, replyTo : ActorRef): Unit = {
    msg match {
      case StorePostReq (userId, userLogin, text, userMentions, token) =>
        this.storePost (userId, userLogin, text, userMentions, token, replyTo)
      case ReadPostReq (post) =>
        this.readPost (post, replyTo)
      case ReadMultiplePostsReq (posts) =>
        this.readMultiplePosts (posts, replyTo)
      case e =>
        log.error (s"Unmanaged message $e")
    }
  }

  /**
    * Store a new post in the database
    */
  def storePost (userId: Int, userLogin : String, text: String, userMentions: List[Int], jwtToken: String, replyTo: ActorRef): Unit = {
    jwtToken match {
      case Jwt (claims) => {
        if (claims.contains ("id") && claims ("id") == userId) {
          val postId = this.postReg.insert (userId, userLogin, text);
          this.userTagReg.insert (userMentions, postId);
          TimelineService.update (this.rqtBuilder, postId, userId, userMentions).value match {
            case Ok (_, _) => {
              replyTo ! true
            }
            case _ => {
              replyTo ! false
            }
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
    * Retreive the content of a post in the database
    */
  def readPost (id: Int, replyTo: ActorRef): Unit = {
    this.postReg.find (id) match {
      case Some ((userId, userLogin, content)) =>
        val userMentions = this.userTagReg.find (id);
        replyTo ! Some (PostContent (id, userId, userLogin, content, userMentions))

      case None =>
        val opt : Option[PostContent] = None
        replyTo ! opt
    }
  }

  /**
    * Retreive posts from their ids
    */
  def readMultiplePosts (ids: List[Int], replyTo: ActorRef): Unit = {
    var lst = ListBuffer[PostContent]()
    for (id <- ids) {
      this.postReg.find (id) match {
        case Some ((userId, userLogin, content)) =>
          val userMentions = this.userTagReg.find (id);
          lst += PostContent (id, userId, userLogin, content, userMentions)
        case None => {}
      }
    }

    replyTo ! lst.toList
  }

}
