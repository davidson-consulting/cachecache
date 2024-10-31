package com.socialnet.timeline.service

import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill  }
import scala.concurrent.duration._
import scala.concurrent.{Await, Future}

import scala.collection.mutable.ListBuffer

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.users.protocol._
import com.socialnet.social_graph.protocol._


import com.socialnet.utils.request._
import com.socialnet.utils.service._
import com.socialnet.utils.response._
import com.socialnet.utils.jwt_tokens._

import com.socialnet.timeline.protocol._
import com.socialnet.timeline.database._
import com.socialnet.timeline.cache._

import com.socialnet.timeline.data.post._
import com.socialnet.timeline.data.home._
import com.socialnet.timeline.data.user._


class TimelineServiceActor (
  name : String,
  config: ServiceConfig,
  rqtCacheConfig : CacheConfig,
  cacheConfig : CacheConfig,
  dbConfig : DBConfig,
  registryConfig: RegistryConfig
)
    extends ServiceActor (TimelineService.NAME, registryConfig, rqtCacheConfig)
{

  case class TreatTimeline ()

  var running = true
  var isBatch = false
  var batchSpeed = 10000

  var syncDBConn : TimelineDatabase = null
  var batchDBConn : TimelineDatabase = null

  var cache : TimelineCache = null

  var syncUserReg : UserTimelineRegistry = null
  var batchUserReg : UserTimelineRegistry = null

  var syncHomeReg : HomeTimelineRegistry = null
  var batchHomeReg : HomeTimelineRegistry = null

  var nbFollowersPerPage = 100
  var updates: Map[Int, Array[UpdateTimelineReq]] = Map ()

  override def postActivation (): Unit = {
    this.syncDBConn = new TimelineDatabase (this.dbConfig)
    this.cache = new TimelineCache (this.cacheConfig)
    this.syncDBConn.connect ()
    this.cache.connect ()

    val ttl = if (this.config.params.contains ("cache-TTL")) {
      this.config.params ("cache-TTL").asInstanceOf [Int]
    } else {
      cacheConfig.TTL
    }

    this.syncUserReg = new UserTimelineRegistry (ttl, this.syncDBConn, this.cache)
    this.syncHomeReg = new HomeTimelineRegistry (ttl, this.syncDBConn, this.cache)

    this.launchBatchIfActivated ()

    if (this.config.params.contains ("followers-per-page")) {
      this.nbFollowersPerPage = this.config.params ("followers-per-page").asInstanceOf [Int]
    }

    if (this.config.params.contains ("batch-speed")) {
      this.batchSpeed = this.config.params ("batch-speed").asInstanceOf [Int]
    }
  }

  override def processMessage (msg: Any, replyTo: ActorRef): Unit = {
    msg match {
      case TreatTimeline () => { // in batch mode, receive this message periodically
        this.treatTimeline ()
      }

      case UpdateTimelineReq (postId, userId, userTags) => // a new post was stored, we need to update timelines
        this.treatTimelineUpdate (postId, userId, userTags, replyTo)

      case HomeTimelineReq (userId, page, nb) =>
        this.findHomeTimeline (userId, page, nb, replyTo)

      case UserTimelineReq (userId, page, nb) =>
        this.findUserTimeline (userId, page, nb, replyTo)

      case HomeTimelineSizeReq (userId) =>
        this.countHomeTimeline (userId, replyTo)

      case UserTimelineSizeReq (userId) =>
        this.countUserTimeline (userId, replyTo)

      case e =>
        log.error (s"Unmanaged message $e")
    }
  }

  /**
    * Send a table containing the post ids of the home timeline of 'userId' to 'replyTo' (paged requests)
    */
  def findHomeTimeline (userId : Int, page : Int, nb : Int, replyTo: ActorRef): Unit = {
    val posts = this.syncHomeReg.select (userId, page, nb).map (_.postId);
    replyTo ! posts
  }

  /**
    * Send a table containing the post ids of the user timeline of 'userId' to 'replyTo' (paged requests)
    */
  def findUserTimeline (userId : Int, page : Int, nb : Int, replyTo: ActorRef): Unit = {
    val posts = this.syncUserReg.select (userId, page, nb).map (_.postId);
    replyTo ! posts
  }

  /**
    * Send the number of posts in the home timeline of 'userId' to 'replyTo'
    */
  def countHomeTimeline (userId : Int, replyTo: ActorRef): Unit = {
    val nb = this.syncHomeReg.count (userId);
    replyTo ! nb
  }

  /**
    * Send the number of posts in the user timeline of 'userId' to 'replyTo'
    */
  def countUserTimeline (userId : Int, replyTo: ActorRef): Unit = {
    val nb = this.syncUserReg.count (userId);
    replyTo ! nb
  }

  /**
    * Treat a request of timeline update, this is done sync or in batch mode depending on the service configuration
    */
  def treatTimelineUpdate (postId: Int, userId: Int, userTags: List[Int], replyTo: ActorRef): Unit = {
    if (this.isBatch) {
      replyTo ! "ACK"

      this synchronized {
        if (this.updates.contains (userId)) {
          this.updates += (userId-> (this.updates (userId) ++ Array (UpdateTimelineReq (postId, userId, userTags))))
        } else {
          this.updates += (userId-> Array (UpdateTimelineReq (postId, userId, userTags)))
        }
      }
    } else {
      val follow = System.currentTimeMillis ();
      val followers = this.getAllFollowers (userId);
      val beg = System.currentTimeMillis ();

      this.syncDBConn.setAutoCommit (false);
      var user = UserInsertStatement ()
      var home = HomeInsertStatement ()

      this.treatImmediately (this.syncUserReg, this.syncHomeReg, postId, userId, userTags, followers, user, home)

      if (user.currentIndex != 0) user.statement.executeBatch ()
      if (home.currentIndex != 0) home.statement.executeBatch ()
      this.syncDBConn.commit ();

      val en = System.currentTimeMillis ();
      this.syncDBConn.setAutoCommit (true);

      replyTo ! "ACK"
    }
  }

  /**
    *  ===========================================================
    *  ===========================================================
    *  ======================     FOLLOWERS    ===================
    *  ===========================================================
    *  ===========================================================
    */

  /**
    * Get the list of followers of 'userId'
    */
  def getAllFollowers (userId: Int): List[Int] = {
    var result = new ListBuffer[Int] ()
    var page = 0
    var stop = false
    while (!stop) {
      val followers = ResponseGetter.getOr (
        SocialGraphService.getFollowers (this.rqtBuilder, userId, page, this.nbFollowersPerPage).value,
        List()
      )

      result ++= followers
      page += 1
      if (followers.size < this.nbFollowersPerPage) stop = true
    }

    result.toList
  }

  /**
    *  ===========================================================
    *  ===========================================================
    *  =======================     BATCH    ======================
    *  ===========================================================
    *  ===========================================================
    */

  /**
    * Launch a future calling the batch treatment if batch mode is activated
    */
  def launchBatchIfActivated (): Unit = {
    if (this.config.params.contains ("type") && this.config.params ("type").asInstanceOf [String] == "batch") {
      this.isBatch = true
      this.batchDBConn = new TimelineDatabase (this.dbConfig);
      this.batchDBConn.connect ();

      val ttl = if (this.config.params.contains ("cache-TTL")) {
        this.config.params ("cache-TTL").asInstanceOf [Int]
      } else {
        cacheConfig.TTL
      }

      this.batchUserReg = new UserTimelineRegistry (ttl, this.batchDBConn, this.cache)
      this.batchHomeReg = new HomeTimelineRegistry (ttl, this.batchDBConn, this.cache)

      this.batchDBConn.setAutoCommit (false);

      log.info ("Timeline service is in batch mode")

      implicit val ec  = scala.concurrent.ExecutionContext.global

      Future {
        while (this.running) {
          Thread.sleep (this.batchSpeed)
          context.self ! TreatTimeline ()
        }
      }
    } else {
      log.info ("Timeline service is in sync mode")
    }
  }

  /**
    * Treat the timeline insertion (in batch mode)
    */
  def treatTimeline (): Unit = {
    implicit val ec  = scala.concurrent.ExecutionContext.global

    Future {
      val toUpdate = this synchronized {
        val toUpdate = this.updates
        this.updates = Map ()
        toUpdate
      }

      if (toUpdate.size != 0) {
        val beg = System.currentTimeMillis ()
        this.syncDBConn.setAutoCommit (false);
        var user = UserInsertStatement ()
        var home = HomeInsertStatement ()
        for (update <- toUpdate) {
          // we get the followers only one time, the result is always the same for all the post of a given user
          val followers = this.getAllFollowers (update._1);
          for (u <- update._2) { // we treat all the post of the user
            this.treatImmediately (this.batchUserReg, this.batchHomeReg, u.postId, update._1, u.userTags, followers, user, home)
          }
        }

        if (user.currentIndex != 0) user.statement.executeBatch ()
        if (home.currentIndex != 0) home.statement.executeBatch ()
        this.batchDBConn.commit ()

        val en = System.currentTimeMillis ()

        log.info (s"Running batch for ${home.count + user.count} insertions took ${en - beg}ms")
      }
    }
  }

  /**
    *  ===========================================================
    *  ===========================================================
    *  =======================     INSERT    =====================
    *  ===========================================================
    *  ===========================================================
    */

  /**
    * Perform the timeline insertion immediately
    */
  def treatImmediately (userReg : UserTimelineRegistry, homeReg : HomeTimelineRegistry, postId: Int, userId: Int, userTags: List[Int], followers: List[Int], userTimeline: UserInsertStatement, homeTimeline: HomeInsertStatement) = {
    userReg.insertBatch (Post (userId, postId), userTimeline)
    homeReg.insertBatch (Post (userId, postId), homeTimeline)

    for (u <- userTags) {
      userReg.insertBatch (Post (u, postId), userTimeline)
      homeReg.insertBatch (Post (u, postId), homeTimeline)
    }

    for (u <- followers) {
      if (!userTags.contains (u)) {
        homeReg.insertBatch (Post (u, postId), homeTimeline)
      }
    }
  }

}
