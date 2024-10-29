package com.socialnet.registry.options

import org.yaml.snakeyaml.Yaml
import java.util.{Map => JMap, List => JArray}
import java.io._
import scala.Array;
import scala.jdk.CollectionConverters._

case class Config (
  addr: String,
  port: Int
)

object Options {

  /**
    * Open and read the configuration file given in parameter
    * If no configuration file is given, read the file "config.yaml"
    */
  def parseOptions (args : Array [String]) : Config = {
    val configFile = if (args.length != 0) {
      args (0)
    } else "resources/services/registry.yaml";

    val yaml : Yaml = new Yaml
    val root = yaml.load (new FileInputStream (new java.io.File (configFile)))
    val node = root.asInstanceOf[JMap[String, Object]].asScala

    val port = node ("port").asInstanceOf [Int]
    val addr = node ("addr").asInstanceOf [String]

    Config (addr, port)
  }

}
