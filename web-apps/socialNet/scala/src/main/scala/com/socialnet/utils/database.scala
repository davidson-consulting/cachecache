package com.socialnet.utils.database

import java.sql.DriverManager
import java.sql.Connection
import scala.reflect.ClassTag

case class SerializationError (reason : String) extends Exception

abstract class TableEntity {
  def tableName (): String;
  def format (sb: StringBuilder): Unit;
}


abstract class SqlDatabase (kind : String, addr : String, port : Int, user : String = "root", password : String = "") {

  var connection : Connection = null
  var url = s"jdbc:${kind}://$addr:$port/${kind}?autoReconnect=true&useSSL=false"

  def connect () = {
    this.connection = DriverManager.getConnection (this.url, user, password)
    this.postConnection ()
  }

  def getConnection (): Connection = {
    this.connection
  }

  def postConnection () = {}

  def createDatabaseIfNotExists (name : String) = {
    var created = false;
    while (!created) {
      try {
        val sql = s"SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = ?";
        val statement = this.connection.prepareStatement (sql);
        statement.setString (1, name);

        val result = statement.executeQuery ();
        if (!result.next ()) {
          val sql_create = s"CREATE DATABASE $name";
          // it does not work with a prepare stmt, and apparently there is no benefit in doing so (according to stackoverflow..)
          val statement_create = this.connection.createStatement ();
          statement_create.executeUpdate (sql_create);
        }
        created = true;
      } catch {
        case e => {
          Thread.sleep (500);
        }
      }
    }

    this.connection.close ();
    this.url = s"jdbc:mysql://$addr:$port/$name?autoReconnect=true&useSSL=false"
    this.connection = DriverManager.getConnection (this.url, user, password);
  }

  def setAutoCommit (b : Boolean) = {
    this.connection.setAutoCommit (b);
  }


  def commit () = {
    this.connection.commit ();
  }


}
