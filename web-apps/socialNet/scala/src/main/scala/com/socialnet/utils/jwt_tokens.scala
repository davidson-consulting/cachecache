package com.socialnet.utils.jwt_tokens

import java.time.Instant
import java.util.{Date, UUID}

import io.jsonwebtoken.{Claims, Jws, Jwts, SignatureAlgorithm}

import scala.jdk.CollectionConverters._

object Jwt {

  val Ttl: Int = 3600
  val Secret: String = "MySecret"

  def apply(claims: Map[String, Any]): String = {
    val jwt = Jwts.builder()
      .setId(UUID.randomUUID.toString)
      .setIssuedAt(Date.from(Instant.now()))
      .setExpiration(Date.from(Instant.now().plusSeconds(Ttl)))
      .signWith(SignatureAlgorithm.HS512, Secret.getBytes("UTF-8"))

    claims.foreach { case (name, value) =>
      jwt.claim(name, value)
    }

    jwt.compact()
  }

  def unapply(jwt: String): Option[Map[String, Any]] =
    try {
      val claims: Jws[Claims] = Jwts.parser()
        .setSigningKey(Secret.getBytes("UTF-8"))
        .parseClaimsJws(jwt) // we can trust this JWT
      Option(claims.getBody.asScala.toMap)
    } catch {
      case _: Exception => None
    }

}
