package com.socialnet.front.main


import akka.actor.{ Props, Actor, ActorSystem => NActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill }
import com.typesafe.config.ConfigFactory

import akka.actor.typed.ActorSystem
import akka.actor.typed.scaladsl.Behaviors

import io.circe.generic.auto._
import io.circe.parser._
import io.circe.syntax._

import akka.http.scaladsl.Http
import akka.http.scaladsl.server.Route
import akka.http.scaladsl.model._
import akka.http.scaladsl.server.Directives._
import scala.io.StdIn

import com.socialnet.front.options._
import com.socialnet.utils.configuration._

import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.utils.request._
import com.socialnet.text.protocol._
import com.socialnet.users.protocol._
import com.socialnet.timeline.protocol.{HomeTimelineSizeReq, UserTimelineSizeReq, TimelineService}
import com.socialnet.social_graph.protocol.{NbFollowers, NbSubscriptions, SocialGraphService, FollowReq, UnFollowReq, IsFollowingReq}
import com.socialnet.compose.protocol._
import com.socialnet.dummy.database._

import com.socialnet.utils.service._
import com.socialnet.utils.response._

import com.socialnet.utils.jwt_tokens._

case class LoginPassword (login: String, password: String)
case class Token (token: String)
case class Submit (user_id: Int, text: String, token: String)
case class Follow (user_id: Int, to_whom: Int, token: String)

object FrontApp {

  var registryActor : ActorSelection = null
  var rqtBuilder : RequestBuilder = null

  def topLevelRoute: Route = {
    concat (
      path ("login")(loginRoute),
      path ("submit")(submitNewPost),
      path ("home-timeline")(homeTimeline),
      path ("home-timeline-len")(homeTimelineLen),
      path ("user-timeline")(userTimeline),
      path ("user-timeline-len")(userTimelineLen),
      path ("subscriptions")(subscriptions),
      path ("subscriptions-len")(subscriptionsLen),
      path ("followers")(followers),
      path ("followers-len")(followersLen),
      path ("follow")(followUser),
      path ("unfollow") (unFollowUser),
      path ("is-following")(isFollowing)
    )
  }

  def loginRoute: Route = {
    post {
      entity(as[String]) { json =>
        decode[LoginPassword](json) match {
          case Right (LoginPassword (login, password)) => {
            UserService.logUser (this.rqtBuilder, login, password).value match {
              case Ok (_, LoginResp (id, _, true, token)) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, Token (token).asJson.noSpaces.toString)))
              }
              case Ok (_,resp) => {
                println ("Error " + resp);
                complete (HttpResponse (401))
              }
              case e => {
                println ("Error : " + e);
                complete (HttpResponse (500))
              }
            }
          }
          case _ =>
            complete (HttpResponse (400))
        }
      }
    }
  }

  def submitNewPost: Route = {
    post {
      entity(as[String]) { json =>
        decode[Submit](json) match {
          case Right (Submit (userId, text, token)) => {
            ComposeService.submitPost (this.rqtBuilder, userId, text, token).value match {
              case Ok (_, true) => {
                complete (HttpResponse (200))
              }
              case Ok (_,_) => {
                complete (HttpResponse (401))
              }
              case _ => {
                complete (HttpResponse (500))
              }
            }
          }
          case _ =>
            complete (HttpResponse (400))
        }
      }
    }
  }

  def homeTimeline: Route = {
    get {
      parameters ("user_id".as[Int], "page".as[Int], "nb".as[Int]) {
        (userId, page, nb) => {
          ComposeService.getHomeTimeline (this.rqtBuilder, userId, page, nb).value match {
            case Ok (_, TimelineResp (Some (lst))) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, lst.asJson.noSpaces.toString)))
              }
            case e  => {
              println ("Error " + e);
                complete (HttpResponse (500))
              }
            }

        }
      }
    }
  }

  def homeTimelineLen: Route = {
    get {
      parameters ("user_id".as[Int]) {
        (userId) => {
          TimelineService.getHomeTimelineSize (this.rqtBuilder, userId).value match {
            case Ok (_, size) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, size.asJson.noSpaces.toString)))
              }
            case e => {
              println ("Error " + e);
                complete (HttpResponse (500))
              }
            }

        }
      }
    }
  }


  def userTimeline: Route = {
    get {
      parameters ("user_id".as[Int], "page".as[Int], "nb".as[Int]) {
        (userId, page, nb) => {
          ComposeService.getUserTimeline (this.rqtBuilder, userId, page, nb).value match {
            case Ok (_, TimelineResp (Some (lst))) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, lst.asJson.noSpaces.toString)))
              }
            case e => {
              println ("Error " + e);
                complete (HttpResponse (500))
              }
            }

        }
      }
    }
  }

  def userTimelineLen: Route = {
    get {
      parameters ("user_id".as[Int]) {
        (userId) => {
          TimelineService.getUserTimelineSize (this.rqtBuilder, userId).value match {
            case Ok (_, size) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, size.asJson.noSpaces.toString)))
              }
            case e => {
              println ("Error " + e);
                complete (HttpResponse (500))
              }
            }

        }
      }
    }
  }

  def subscriptions: Route = {
    get {
      parameters ("user_id".as[Int], "page".as[Int], "nb".as[Int]) {
        (userId, page, nb) => {
          ComposeService.getSubscriptions (this.rqtBuilder, userId, page, nb).value match {
            case Ok (_, FollowOrSubsResp (Some (lst))) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, lst.asJson.noSpaces.toString)))
              }
              case _ => {
                complete (HttpResponse (500))
              }
            }

        }
      }
    }
  }

  def subscriptionsLen: Route = {
    get {
      parameters ("user_id".as[Int]) {
        (userId) => {
          SocialGraphService.getNbSubscriptions (this.rqtBuilder, userId).value match {
            case Ok (_, size) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, size.asJson.noSpaces.toString)))
              }
              case _ => {
                complete (HttpResponse (500))
              }
            }

        }
      }
    }
  }

  def followers: Route = {
    get {
      parameters ("user_id".as[Int], "page".as[Int], "nb".as[Int]) {
        (userId, page, nb) => {
          ComposeService.getFollowers (this.rqtBuilder, userId, page, nb).value match {
            case Ok (_, FollowOrSubsResp (Some (lst))) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, lst.asJson.noSpaces.toString)))
              }
              case _ => {
                complete (HttpResponse (500))
              }
            }

        }
      }
    }
  }

  def followersLen: Route = {
    get {
      parameters ("user_id".as[Int]) {
        (userId) => {
          SocialGraphService.getNbFollowers (this.rqtBuilder, userId).value match {
            case Ok (_, size) => {
                complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, size.asJson.noSpaces.toString)))
              }
              case _ => {
                complete (HttpResponse (500))
              }
            }

        }
      }
    }
  }

  def followUser: Route = {
    post {
      entity(as[String]) { json =>
        decode[Follow](json) match {
          case Right (Follow (userId, toWhom, token)) => {
            SocialGraphService.follow (this.rqtBuilder, userId, toWhom, token).value match {
              case Ok (_, true) => {
                complete (HttpResponse (200))
              }
              case Ok (_, false) => {
                complete (HttpResponse (401))
              }
              case _ => {
                complete (HttpResponse (500))
              }
            }
          }
          case _ =>
            complete (HttpResponse (400))
        }
      }
    }
  }

  def unFollowUser: Route = {
    post {
      entity(as[String]) { json =>
        decode[Follow](json) match {
          case Right (Follow (userId, toWhom, token)) => {
            SocialGraphService.unfollow (this.rqtBuilder, userId, toWhom, token).value match {
              case Ok (_, true) => {
                complete (HttpResponse (200))
              }
              case Ok (_, false) => {
                complete (HttpResponse (401))
              }
              case _ => {
                complete (HttpResponse (500))
              }
            }
          }
          case e =>
            complete (HttpResponse (400))
        }
      }
    }
  }

  def isFollowing: Route = {
    get {
      parameters ("user_id".as[Int], "to_whom".as[Int]) {
        (userId, toWhom) => {
          SocialGraphService.isFollowing (this.rqtBuilder, userId, toWhom).value match {
            case Ok (_, b) => {
              complete (HttpResponse (200, entity = HttpEntity(ContentTypes.`application/json`, b.asJson.noSpaces.toString)))
            }
            case _ => {
              complete (HttpResponse (500))
            }
          }
        }
      }
    }
  }

  def main(args: Array[String]): Unit = {
    val actorConfig = Options.parseOptions (args)
    val sysConf = ConfigFactory.parseString (
      ConfigLoader.readConfiguration ("resources/master.conf",
        Map ("addr"-> actorConfig.addr, "port"-> actorConfig.port.toString)
      )
    );

    val registrySystem = NActorSystem ("RemoteSystem", sysConf)
    this.registryActor = registrySystem.actorSelection (s"akka://RemoteSystem@${actorConfig.registry.addr}:${actorConfig.registry.port}/user/registry")
    this.rqtBuilder = new RequestBuilder (registryActor, null)

    implicit val system = ActorSystem(Behaviors.empty, "http-system")
    implicit val executionContext = system.executionContext

    val bindingFuture = Http().newServerAt("0.0.0.0", actorConfig.httpPort).bind(topLevelRoute)

    println(s"Server now online. Please navigate to http://localhost:${actorConfig.httpPort}/hello\nPress RETURN to stop...")
    StdIn.readLine() // let it run until user presses return
    bindingFuture
      .flatMap(_.unbind()) // trigger unbinding from the port
      .onComplete(_ => system.terminate()) // and shutdown when done
  }
}
