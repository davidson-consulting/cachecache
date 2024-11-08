package socialnetwork

import io.gatling.core.Predef._
import io.gatling.http.Predef._

import java.util.concurrent.ThreadLocalRandom

import scala.util.Random
import scala.util.matching._

/**
  * This sample is based on our official tutorials:
  *
  *   - [[https://gatling.io/docs/gatling/tutorials/quickstart Gatling quickstart tutorial]]
  *   - [[https://gatling.io/docs/gatling/tutorials/advanced Gatling advanced tutorial]]
  */
class SocialNetworkSimulation extends Simulation {

  val SEED = 0
  val randAlpha = new Random (SEED).alphanumeric
  val randNumber = new Random (SEED)
  val NB_USERS = 20000;
  val NB_NEW_POSTS = 100
  val NB_READS = 10
  val NB_AT_ONCE = 10
  val PAGE_SIZE = 50
  val SIM_SIZE = 1

  val feeder = initGlobals ()

  /**
    *  ==================================================================================
    *  ==================================================================================
    *  =======================             GLOBALS               ========================
    *  ==================================================================================
    *  ==================================================================================
    */

  def initGlobals () = {
    var lst : List[Map[String, String]] = List ()
    for (i <- 1 to NB_USERS) {
      lst = lst :+ Map("user_id" -> s"$i", "user_name" -> s"user_$i")
    }

    val feeder = lst.iterator
    (feeder)
  }

  /**
    *  ==================================================================================
    *  ==================================================================================
    *  =====================             EXTRACTIONS               ======================
    *  ==================================================================================
    *  ==================================================================================
    */

  def extractAddresses (arr : Vector[String]) = {
    var result : Vector[String] = Vector()
    var users : Vector[String] = Vector()

    val pattern = "<a href=([^>])*>".r
    for (i <- arr) {
      val matches = pattern.findAllMatchIn (i)

      for (a <- matches) {
        val ad = a.toString ()
        if (ad.contains ("/user/")) {
          val start = ad.indexOf ("user/");
          val userId = ad.substring (start + 5, ad.length () - 3)
          if (randNumber.nextInt (20) <= 1) {
            users = users :+ userId
          }
        } else if (ad.contains ("/api/url")) {
          result = result :+ ad.substring (ad.indexOf ("/api/"), ad.length () - 3)
        }
      }
    }

    (result, users)
  }

  /**
    *  ==================================================================================
    *  ==================================================================================
    *  =======================             POST GEN               =======================
    *  ==================================================================================
    *  ==================================================================================
    */

  /**
    * Select N id in M user
    */
  def selectNInM (n : Int, m : Int) = {
    var res : Vector[Int] = Vector()
    for (i <- 0 until n) {
      res = res :+ randNumber.nextInt (m)
    }

    res
  }

  def randomString(length: Int) = {
    var sb = new StringBuilder ("");
    var allLen = 0
    while (allLen < length) {
      sb ++= randNumber.nextString (20).filter (_.isLetter) + " "
      allLen += 20
    }
    sb.toString
  }

  def generateNewPost () = {
    var len = randNumber.nextInt (50) + 5
    var b = randomString (len)

    len = randNumber.nextInt (50) + 5
    val e = randomString (len)

    val users = selectNInM (3, NB_USERS)
    for (u <- users) {
      val name = s"user_$u"
      b = b + " @" + name + " "
    }

    b + " " + e + " https://github.com/GNU-Ymir/bootstrap"
  }

  def randomItems () = {
    20
  }


  /**
    *  ==================================================================================
    *  ==================================================================================
    *  =======================               TASKS                =======================
    *  ==================================================================================
    *  ==================================================================================
    */


  def readHomeTimelineTask () = {
    feed (feeder)
      .repeat (NB_READS, "page") {
        exec { session =>
          var sess = session.set ("items", PAGE_SIZE)
          sess = sess.set ("page_index", randNumber.nextInt (10))
          sess
        }
          .exec (
            http ("ReadTimeline")
              .get ("/home-timeline?user_id=#{user_id}&page=#{page_index}&nb=#{items}")
              .check (
                bodyString.saveAs ("truc"),
                jsonPath ("$[*].text").findRandom (5).saveAs ("texts")
              )
          )
          .exec { session =>
            if (session.contains ("texts")) {
              val (addrs, users) = extractAddresses (session ("texts").as [Vector[String]])
              var newSession = session.set ("users", users)
              newSession
            } else {
              session.set ("users", Vector())
            }
          }
          .foreach ("#{users}", "user") {
            exec {
              http ("ReadUser")
                .get ("/user-timeline?user_id=#{user}&page=0&nb=#{items}")
                .check (status.is (200))
            }
          }
      }
  }

  def postNewPostTask () = {
    feed (feeder)
      .exec (
        http ("login")
          .post ("/login")
          .header ("Content-type", "application/json")
          .body (StringBody ("{\"login\" : \"#{user_name}\", \"password\" : \"pass\"}"))
          .check (
            jsonPath ("$.token").saveAs ("token"),
            jsonPath ("$.user_id").saveAs ("uid")
          )
      )
      .repeat (NB_NEW_POSTS) {
        exec { session =>
          var newSession = session.set ("post", generateNewPost ())
          newSession
        }
          .exec (
            http ("submit")
              .post ("/submit")
              .header ("Content-type", "application/json")
              .body (StringBody ("{\"user_id\" : #{uid}, \"token\" : \"#{token}\", \"text\" : \"#{post}\"}"))
              .check (
                status.is (200)
              )
          )
      }
      .pause (1)
  }

  /*!
  * ====================================================================================================
  * ====================================================================================================
  * ======================================          MAIN          ======================================
  * ====================================================================================================
  * ====================================================================================================
  */

  val readHome = readHomeTimelineTask ();
  val newPost = postNewPostTask ();

  val httpProtocol =
    http.baseUrl("http://127.0.0.1:8080")
      .acceptHeader("text/html,application/xhtml+xml,application/xml, application/json;q=0.9,*/*;q=0.8")
      .acceptLanguageHeader("en-US,en;q=0.5")
      .acceptEncodingHeader("gzip, deflate")
      .userAgentHeader(
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:16.0) Gecko/20100101 Firefox/16.0"
      );


  val write = scenario ("Users").exec (newPost);
  val read = scenario ("Read").exec (readHome);

  setUp (
    write.inject (
      constantConcurrentUsers(NB_AT_ONCE).during(SIM_SIZE)
      // nothingFor (2),
      // atOnceUsers (NB_AT_ONCE)
    )
  ).protocols (httpProtocol)

}
