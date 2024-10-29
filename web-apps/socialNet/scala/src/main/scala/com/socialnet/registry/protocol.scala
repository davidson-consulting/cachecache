package com.socialnet.registry.protocol

import akka.actor.{ActorRef}
import com.socialnet.utils.serializer._

case class RegisterService (
  kind: String,
  token: String,
  actor : ActorRef
) extends DefaultSerializer

case class UnregisterService (
  kind: String,
  token: String,
  actor : ActorRef
) extends DefaultSerializer

case class RegisterResponse (
  id: String,
  msg: String,
  token: String
) extends DefaultSerializer

case class UnregisterResponse (
  msg: String
) extends DefaultSerializer

case class ServiceAccessReq (
  kind: String
) extends DefaultSerializer

case class ServiceAccessResp (
  actor: Option [ActorRef]
) extends DefaultSerializer

case class RegistryDied (
  token: String
) extends DefaultSerializer
