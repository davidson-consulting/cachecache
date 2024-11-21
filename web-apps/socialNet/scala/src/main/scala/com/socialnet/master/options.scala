package com.socialnet.master.options

import org.yaml.snakeyaml.Yaml
import java.util.{Map => JMap, List => JArray}
import scala.jdk.CollectionConverters._
// import scala.collection.JavaConverters._
import java.io._
import scala.Array;

case class ServiceConfig (
  service: String,
  cache: String,
  rqt_cache: String,
  db: String,
  params: Map[String, Any],
  replicate: Int
)

case class DBConfig (
  kind: String,
  addr: String,
  user: String,
  password: String,
  port: Int
)

case class CacheConfig (
  kind: String,
  addr: String,
  port: Int,
  TTL: Int
)

case class RegistryConfig (
  addr: String,
  port: Int
)

case class Config (
  services: Map[String, ServiceConfig],
  registry: RegistryConfig,
  caches: Map[String, CacheConfig],
  dbs: Map[String, DBConfig],
  addr: String,
  port: Int
)

object Options {

  /**
    * Read the configuration file of the services
    * @params:
    *    - values: the list of services in the yaml file
    */
  def readServices (values : Object) : Map[String, ServiceConfig] = {
    var serviceConfig : Map[String, ServiceConfig] = Map ()

    for (service_kv <- values.asInstanceOf [JMap[String, Object]].asScala) {
      val service = service_kv._2.asInstanceOf [JMap[String, Object]].asScala
      val name = service_kv._1
      val serviceType = service ("type").asInstanceOf [String]
      val cache =
        if (service.contains ("cache")) {
          service ("cache").asInstanceOf [String]
        }
        else "no";

      val db =
        if (service.contains ("db")) { service ("db").asInstanceOf [String] }
        else "no";

      val rqt_cache =
        if (service.contains ("rqt-cache")) { service ("rqt-cache").asInstanceOf [String] }
        else "no";

      val replicate =
        if (service.contains ("replicate")) { service ("replicate").asInstanceOf [Int] }
        else 1;

      val params : Map[String, Any] =
        if (service.contains ("params")) { Map (service ("params").asInstanceOf [JMap[String, Any]].asScala.toSeq:_*) }
        else Map ()

      serviceConfig += (name -> ServiceConfig (serviceType, cache, rqt_cache, db, params, replicate))
    }

    serviceConfig
  }

  /**
    * Read the configuration file of the caches
    * @params:
    *     - values: the list of caches in the yaml file
    */
  def readCacheConfig (values : Object) : Map[String, CacheConfig] = {
    var cacheConfig : Map [String, CacheConfig] = Map ()

    for (cache_kv <- values.asInstanceOf [JMap[String, Object]].asScala) {
      val cache = cache_kv._2.asInstanceOf [JMap[String, Object]].asScala
      val name = cache_kv._1

      val t = cache ("type").asInstanceOf [String]
      val addr = cache ("addr").asInstanceOf [String]
      val port = cache ("port").asInstanceOf [Int]
      val ttl = if (cache.contains ("TTL")) {
        cache ("TTL").asInstanceOf [Int]
      } else {
        60
      }

      cacheConfig += (name -> CacheConfig (t, addr, port, ttl))
    }

    cacheConfig
  }

  /**
    * Read the configuration file of the dbs
    * @params:
    *     - values: the list of dbs in the yaml file
    */
  def readDBConfig (values : Object) : Map[String, DBConfig] = {
    var dbConfig : Map [String, DBConfig] = Map ()

    for (db_kv <- values.asInstanceOf [JMap[String, Object]].asScala) {
      val db = db_kv._2.asInstanceOf [JMap[String, Object]].asScala
      val name = db_kv._1

      val addr = db ("addr").asInstanceOf [String]
      val port = db ("port").asInstanceOf [Int]

      val kind = if (db.contains ("kind")) db ("kind").asInstanceOf [String] else "mysql"
      val user = if (db.contains ("user")) db ("users").asInstanceOf [String] else "root"
      val password = if (db.contains ("password")) db ("password").asInstanceOf [String] else ""

      dbConfig += (name -> DBConfig (kind, addr, user, password, port))
    }

    dbConfig
  }

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
    } else "resources/services/config.yaml";


    val yaml : Yaml = new Yaml
    val root = yaml.load (new FileInputStream (new java.io.File (configFile)))
    val node = root.asInstanceOf[JMap[String, Object]].asScala

    val serviceConfig = this.readServices (node ("services"))
    val cacheConfig = if (node.contains ("caches")) { this.readCacheConfig (node ("caches")) } else Map ();
    val dbConfig = if (node.contains ("databases")) { this.readDBConfig (node ("databases")) } else Map ();
    val registry = this.readRegistry (node ("registry"))
    val port = node ("port").asInstanceOf [Int]
    val addr = node ("addr").asInstanceOf [String]

    Config (serviceConfig, registry, cacheConfig, dbConfig, addr, port)
  }

}
