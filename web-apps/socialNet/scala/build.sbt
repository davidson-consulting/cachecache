
name := "socialnetwork"
version := "1.0"
scalaVersion := "3.2.2"
val AkkaVersion = "2.8.3"
val AkkaHttpVersion = "10.5.2"
val circeVersion = "0.14.1"
val gatlingVersion = "3.9.5"

scalacOptions := Seq(
  "-encoding", "UTF-8", "-release:8", "-deprecation",
  "-feature", "-unchecked", "-language:implicitConversions", "-language:postfixOps")

resolvers += "Akka library repository".at("https://repo.akka.io/maven")

libraryDependencies += "ch.qos.logback" % "logback-classic" % "1.1.3" % Runtime
libraryDependencies += "com.typesafe.akka" %% "akka-actor-typed" % AkkaVersion
libraryDependencies += "com.typesafe.akka" %% "akka-remote" % AkkaVersion
libraryDependencies += "com.typesafe.akka" %% "akka-http" % AkkaHttpVersion

libraryDependencies += "com.typesafe.akka" %% "akka-serialization-jackson" % AkkaVersion

libraryDependencies += "org.yaml" % "snakeyaml" % "1.8"
libraryDependencies += "mysql" % "mysql-connector-java" % "8.0.11"

libraryDependencies += "org.mindrot"  % "jbcrypt"   % "0.3m"
libraryDependencies += "io.jsonwebtoken" % "jjwt" % "0.9.1"
libraryDependencies += "javax.xml.bind" % "jaxb-api" % "2.3.1"

libraryDependencies += "redis.clients" % "jedis" % "4.3.1"

libraryDependencies += "com.typesafe.scala-logging" %% "scala-logging" % "3.9.4"

libraryDependencies ++= Seq(
  "io.circe" %% "circe-core",
  "io.circe" %% "circe-generic",
  "io.circe" %% "circe-parser"
).map(_ % circeVersion)

assemblyMergeStrategy in assembly := {
  case PathList("META-INF", _*) => MergeStrategy.last
  case PathList("module-info.class", _*) => MergeStrategy.last
  case other => MergeStrategy.defaultMergeStrategy(other)
}
