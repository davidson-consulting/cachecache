package com.socialnet.utils.configuration

import org.yaml.snakeyaml.Yaml
import java.util.{Map => JMap, List => JArray}
import scala.jdk.CollectionConverters._
import scala.io.Source
import scala.Array;


object ConfigLoader {

  def readConfiguration (filename : String, values : Map[String, String]): String = {
    var fileContents = Source.fromFile(filename).getLines.mkString ("\n")
    for (i <- values) {
      fileContents = fileContents.replace (s"{{${i._1}}}", i._2)
    }


    fileContents
  }


}
