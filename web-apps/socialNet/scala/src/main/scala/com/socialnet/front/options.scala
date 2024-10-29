package com.socialnet.front.options

import org.yaml.snakeyaml.Yaml
import java.util.{Map => JMap, List => JArray}
import java.io._
import scala.Array;
import scala.jdk.CollectionConverters._


case class RegistryConfig (
  addr: String,
  port: Int
)

case class Config (
  registry: RegistryConfig,
  addr: String,
  port: Int,
  httpPort: Int
)

object Options {

  /**
    * Read the configuration of the registry
    */
  def readRegistry (values : Object) : RegistryConfig = {
    val valuesMap = values.asInstanceOf [JMap[String, Object]].asScala;
    RegistryConfig (valuesMap ("addr").asInstanceOf [String], valuesMap ("port").asInstanceOf [Int])
  }

  /**
    * Open and read the configuration file given in parameter
    * If no configuration file is given, read the file "config.yaml"
    */
  def parseOptions (args : Array [String]) : Config = {
    val configFile = if (args.length != 0) {
      args (0)
    } else "resources/services/front.yaml";

    val yaml : Yaml = new Yaml
    val root = yaml.load (new FileInputStream (new java.io.File (configFile)))
    val node = root.asInstanceOf[JMap[String, Object]].asScala

    val registry = this.readRegistry (node ("registry"))
    val port = node ("port").asInstanceOf [Int]
    val httpPort = node ("httpPort").asInstanceOf [Int]
    val addr = node ("addr").asInstanceOf [String]

    Config (registry, addr, port, httpPort)
  }

}
