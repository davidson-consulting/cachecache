package com.socialnet.text.service

import akka.actor.{Props, Actor, ActorSystem, ActorRef, ActorLogging , ActorSelection, PoisonPill  }

import scala.collection.mutable.ListBuffer

import com.socialnet.master._
import com.socialnet.master.options._
import com.socialnet.registry._
import com.socialnet.registry.protocol._

import com.socialnet.utils.request._
import com.socialnet.text.protocol._
import com.socialnet.dummy.database._
import com.socialnet.users.protocol._
import com.socialnet.short_url.protocol._

import com.socialnet.utils.service._
import com.socialnet.utils.actor_request._
import com.socialnet.utils.response._

import java.util.regex.{Matcher, Pattern}

class TextServiceActor (
  name : String,
  config: ServiceConfig,
  rqtCacheConfig : CacheConfig,
  registryConfig: RegistryConfig
)
    extends ServiceActor (TextService.NAME, registryConfig, rqtCacheConfig)
{

  override def postActivation() : Unit = {
  }

  override def processMessage (msg : Any, replyTo : ActorRef): Unit = {
    msg match {
      case TextRequest (text) =>
        log.debug (s"Receive text to construct : $text")
        this.constructText (text, replyTo)
      case e =>
        log.error (s"Unmanaged message $e")
    }
  }

  /**
    * Answering a TextRequest
    * @params:
    *    - text: the text to transform
    *    - replyTo: the actor requesting the transformation
    */
  def constructText (text : String, replyTo : ActorRef): Unit = {
    val usersMentions = this.findUserMentions (text)
    val urlMentions   = this.findUrlMentions (text)

    // Prepare the requests to the other services
    val userReq = UserService.getIds (this.rqtBuilder, usersMentions).start ()
    var request : Map[String, Request[LongToShortResp]] = Map ()
    for (url <- urlMentions) {
      request += (url -> (ShortUrlService.shorten (this.rqtBuilder, url).start ()))
    }

    // Getting the results of the requests
    val userTags = ResponseGetter.getOr (userReq.value, UserIdsResp (Map ())).ids
    var shortUrls: Map[String, String] = Map ()
    for (r <- request) {
      val result = ResponseGetter.getOr (r._2.value, LongToShortResp (r._1))
      shortUrls += (r._1-> result.short)
    }

    // Use the requests result to transform the text
    val resultText = this.constructNewText (text, shortUrls, userTags)
    replyTo ! TextResponse (resultText, userTags)
  }


  /**
    * Transform the result text, by replacing long url to short urls, and user mention to link to user pages
    * @params:
    *    - old: the old text to transform
    *    - shortUrls: the list of short urls (long-> short)
    *    - userTags: the list of user tags (login-> id)
    */
  def constructNewText (old: String, shortUrls: Map[String, String], userTags: Map[String, Int]): String = {
    var url: Map[String, Int] = Map()
    var newMessage = old

    var pattern = Pattern.compile("(http://|https://)([a-zA-Z0-9_!~*'().&=+$%-/]+)");
    var matcher = pattern.matcher(newMessage);
    var stop = false

    while (matcher.find () && !stop) {
      if (shortUrls.contains (matcher.group ())) {
        val sh = "<a href=\"{{FRONT}}/url/" + shortUrls (matcher.group ()) + "\">http://shorturl/" + shortUrls (matcher.group ()) + "</a>"

        newMessage = newMessage.substring (0, matcher.start ())
        + sh
        + this.constructNewText (newMessage.substring (matcher.start () + matcher.group ().length), shortUrls, userTags);
        stop = true;
      }
    }

    pattern = Pattern.compile("@[a-zA-Z0-9-_]+");
    matcher = pattern.matcher(newMessage);
    stop = false
    while (matcher.find () && !stop) {
      if (userTags.contains (matcher.group ().substring (1))) {
        val sh = "<a href=\"{{FRONT}}/user/" + userTags (matcher.group ().substring (1)) + ">@" + matcher.group ().substring (1) + "</a>"

        newMessage = newMessage.substring (0, matcher.start ())
        + sh
        + this.constructNewText (newMessage.substring (matcher.start () + matcher.group ().length), shortUrls, userTags);
        stop = true
      }
    }

    newMessage
  }

  /**
    * Traverse the message looking for user mentions
    * @returns: the user mentions found in the text (e.g. @user_name)
    */
  def findUserMentions (text : String): List[String] = {
    var lst = new ListBuffer[String] ()

    val pattern = Pattern.compile("@[a-zA-Z0-9-_]+");
    val matcher = pattern.matcher(text);
    while (matcher.find()) {
      lst += matcher.group().substring(1)
    }

    lst.toList
  }

  /**
    * Traverse the message looking for http/https urls
    * @returns: the urls found in the text
    */
  def findUrlMentions (text: String): List[String] = {
    var lst = new ListBuffer[String] ()

    val pattern = Pattern.compile("(http://|https://)([a-zA-Z0-9_!~*'().&=+$%-/]+)");
    val matcher = pattern.matcher(text);
    while (matcher.find()) {
      lst += matcher.group();
    }

    lst.toList
  }

}
