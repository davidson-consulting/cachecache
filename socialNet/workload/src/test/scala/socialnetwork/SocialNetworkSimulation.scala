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
  val randNumber = Array (new Random (SEED),
    new Random (SEED),
    new Random (SEED),
    new Random (SEED))

  var currentUser = Array (0, 0, 0, 0);

  val NB_USERS = 20000;
  val NB_USERS_SUBMIT = 2000;
  val NB_AT_ONCE = 64
  val PAGE_SIZE = 100
  val SIM_SIZE = 100

  /**
    *  ==================================================================================
    *  ==================================================================================
    *  =====================             EXTRACTIONS               ======================
    *  ==================================================================================
    *  ==================================================================================
    */

  def extractAddresses (index : Int, arr : Vector[String]) = {
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
          if (randNumber (0).nextInt (20) <= 1) {
            users = users :+ userId
          }
        } else if (ad.contains ("/api/url")) {
          result = result :+ ad.substring (ad.indexOf ("/api/"), ad.length () - 3)
        }
      }
    }

    (result, users)
  }

  def getNextUser (i : Int) = {
    this.synchronized {
      currentUser (i) = (currentUser (i) + 1) % NB_USERS;
      currentUser (i)
    }
  }

  def getNbNewPosts (i : Int) = {
    if (i < 500) {
      100
    } else {
      randNumber (0).nextInt (70) + 10
    }
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
      res = res :+ randNumber (0).nextInt (m)
    }

    res
  }

  def randomString(length: Int) = {
    var sb = new StringBuilder ("");
    var allLen = 0
    while (allLen < length) {
      sb ++= randNumber (0).nextString (20).filter (_.isLetter) + " "
      allLen += 20
    }
    sb.toString
  }

  def generateNewPost () = {
    var len = randNumber (0).nextInt (50) + 5
    var b = randomString (len)

    len = randNumber (0).nextInt (50) + 5
    val e = randomString (len)

    val users = selectNInM (3, NB_USERS_SUBMIT)
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


  def readHomeTimelineTask (i : Int) = {
    exec { session =>
      val user = randNumber (i).nextInt (10000);
      var sess = session.set ("items", PAGE_SIZE);
      sess = sess.set ("page_index", 0);
      sess = sess.set ("user_rand", user);
      sess
    }
      .exec (
        http (s"ReadTimeline-${i}")
          .get ("/home-timeline?user_id=#{user_rand}&page=#{page_index}&nb=#{items}")
          .check (
            bodyString.saveAs ("truc"),
            jsonPath ("$[*].text").findRandom (5, failIfLess = false).saveAs ("texts")
          )
      )
      .exec { session =>
        if (session.contains ("texts")) {
          val (addrs, users) = extractAddresses (i, session ("texts").as [Vector[String]])
          var newSession = session.set ("users", users)
          newSession
        } else {
          session.set ("users", Vector())
        }
      }
      .foreach ("#{users}", "user") {
        exec {
          http (s"ReadUser-${i}")
            .get ("/user-timeline?user_id=#{user}&page=0&nb=#{items}")
            .check (status.is (200))
        }
      }
  }

  def postNewPostTask (i : Int) = {
    exec { session =>
      val user = getNextUser (i);
      val nb = getNbNewPosts (user);
      println (s"Submit user : ${user} : ${nb}")

      var sess = session.set ("user_name", s"user_${user}");
      sess = sess.set ("nb_new", s"${nb}");
      sess = sess.set ("user_id", s"${user}");
      sess
    }
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
      .repeat ("#{nb_new}") {
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
  }

  /*!
  * ====================================================================================================
  * ====================================================================================================
  * ======================================          MAIN          ======================================
  * ====================================================================================================
  * ====================================================================================================
  */

  val httpProtocol =
    http.baseUrl("http://192.168.1.16:8081")
      .acceptHeader("text/html,application/xhtml+xml,application/xml, application/json;q=0.9,*/*;q=0.8")
      .acceptLanguageHeader("en-US,en;q=0.5")
      .acceptEncodingHeader("gzip, deflate")
      .userAgentHeader(
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:16.0) Gecko/20100101 Firefox/16.0"
      );


  val httpProtocol2 =
    http.baseUrl("http://192.168.1.16:8082")
      .acceptHeader("text/html,application/xhtml+xml,application/xml, application/json;q=0.9,*/*;q=0.8")
      .acceptLanguageHeader("en-US,en;q=0.5")
      .acceptEncodingHeader("gzip, deflate")
      .userAgentHeader(
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:16.0) Gecko/20100101 Firefox/16.0"
      );

  val httpProtocol3 =
    http.baseUrl("http://192.168.1.16:8083")
      .acceptHeader("text/html,application/xhtml+xml,application/xml, application/json;q=0.9,*/*;q=0.8")
      .acceptLanguageHeader("en-US,en;q=0.5")
      .acceptEncodingHeader("gzip, deflate")
      .userAgentHeader(
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:16.0) Gecko/20100101 Firefox/16.0"
      );

  val httpProtocol4 =
    http.baseUrl("http://192.168.1.16:8084")
      .acceptHeader("text/html,application/xhtml+xml,application/xml, application/json;q=0.9,*/*;q=0.8")
      .acceptLanguageHeader("en-US,en;q=0.5")
      .acceptEncodingHeader("gzip, deflate")
      .userAgentHeader(
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:16.0) Gecko/20100101 Firefox/16.0"
      );


  val write = scenario ("Write").exec (repeat (NB_USERS_SUBMIT / NB_AT_ONCE) (postNewPostTask (0)));
  val write2 = scenario ("Write2").exec (repeat (NB_USERS_SUBMIT / NB_AT_ONCE) (postNewPostTask (1)));
  val write3 = scenario ("Write3").exec (repeat (NB_USERS_SUBMIT / NB_AT_ONCE) (postNewPostTask (2)));
  val write4 = scenario ("Write4").exec (repeat (NB_USERS_SUBMIT / NB_AT_ONCE) (postNewPostTask (3)));

  val read = scenario ("Read").exec (repeat (100) (readHomeTimelineTask (0)));
  val read2 = scenario ("Read2").exec (repeat (100) (readHomeTimelineTask (1)));
  val read3 = scenario ("Read3").exec (repeat (100) (readHomeTimelineTask (2)));
  val read4 = scenario ("Read4").exec (repeat (100) (readHomeTimelineTask (3)));

  setUp (
    read.inject (
        atOnceUsers (NB_AT_ONCE)
    ).protocols (httpProtocol)

    , read2.inject (
      nothingFor (40),
      atOnceUsers (NB_AT_ONCE)
    ).protocols (httpProtocol2)

    , read3.inject (
      nothingFor (40),
      atOnceUsers (NB_AT_ONCE)
    ).protocols (httpProtocol3)

    , read4.inject (
      nothingFor (70),
      atOnceUsers (NB_AT_ONCE)
    ).protocols (httpProtocol4)
  )

  // setUp (
  //   write.inject (
  //     atOnceUsers (NB_AT_ONCE)
  //   ).protocols (httpProtocol)

  // , write2.inject (
  //   atOnceUsers (NB_AT_ONCE)
  // ).protocols (httpProtocol2)

  // , write3.inject (
  //   atOnceUsers (NB_AT_ONCE)
  // ).protocols (httpProtocol3)

  // , write4.inject (
  //   atOnceUsers (NB_AT_ONCE)
  // ).protocols (httpProtocol4)
  //  )

}
