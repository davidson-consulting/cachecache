package com.socialnet.dummy.database

import com.socialnet.utils.database._
import java.sql.Connection

case class DummyTableOne (val id: Int, val name: String)

object DummyTableOne {

  def createTable (connect : Connection) = {
    val sql = "CREATE TABLE IF NOT EXISTS DummyTableOne (\n"
    + "    id INT PRIMARY KEY NOT NULL,\n"
    + "    name VARCHAR (255)\n)";

    val statement = connect.createStatement ();
    statement.executeUpdate (sql);
  }

  def insert (connect: Connection, lst : List[DummyTableOne]) = {
    val statement = connect.prepareStatement ("insert into DummyTableOne values (?, ?)");
    var j = 0;
    for (i <- lst) {
      statement.setInt (1, i.id);
      statement.setString (2, i.name);
      statement.addBatch ();

      j += 1;
      if (j % 1000 == 0 || j == lst.size) {
        statement.executeBatch ();
      }
    }
  }

  def select (connect : Connection, page : Int, nb : Int): List[DummyTableOne] = {
    val statement = connect.prepareStatement ("select * from DummyTableOne order by id limit ? offset ?")
    statement.setInt (1, nb);
    statement.setInt (2, nb * page);

    val res = statement.executeQuery ();
    var lst : List [DummyTableOne] = List ()
    while (res.next ()) {
      lst = lst ++ List (DummyTableOne (res.getInt ("id"), res.getString ("name")));
    }

    lst
  }

}

class DummyDatabase (addr : String = "localhost", port : Int = 3306, user : String = "root", password : String = "")
    extends SqlDatabase ("mysql", addr, port, user, password) {

  override def postConnection () = {
    this.createDatabaseIfNotExists ("Dummy");
    DummyTableOne.createTable (this.connection);

    // DummyTableOne.insert (this.connection,
    //   List (
    //     DummyTableOne (4, "test1"),
    //     DummyTableOne (5, "test4"),
    //     DummyTableOne (6, "test9")
    //   )
    // )

    for (i <- DummyTableOne.select (this.connection, 0, 3)) {
      println (i);
    }
    println ("Page 2");
    for (i <- DummyTableOne.select (this.connection, 1, 3)) {
      println (i);
    }

  }

}
