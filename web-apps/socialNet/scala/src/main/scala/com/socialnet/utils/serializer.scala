package com.socialnet.utils.serializer

import java.io.{ByteArrayInputStream, ByteArrayOutputStream, ObjectInputStream, ObjectOutputStream}
import java.util.Base64
import java.nio.charset.StandardCharsets.UTF_8

import scala.reflect.ClassTag

trait DefaultSerializer
trait JsonSerializer

object CacheSerial extends App {

  def serialize (value: Any): String = {
    val stream: ByteArrayOutputStream = new ByteArrayOutputStream()
    val oos = new ObjectOutputStream(stream)
    oos.writeObject(value)
    oos.close
    new String(
      Base64.getEncoder().encode(stream.toByteArray),
      UTF_8
    )
  }

  def deserialize [T: ClassTag](str: String): Option[T] = {
    val bytes = Base64.getDecoder().decode(str.getBytes(UTF_8))
    val ois = new ObjectInputStream(new ByteArrayInputStream(bytes))
    val value = ois.readObject
    ois.close

    Some (value.asInstanceOf [T])
  }

}
