package com.socialnet.users.database

import org.mindrot.jbcrypt.BCrypt

import com.socialnet.utils.database._
import com.socialnet.master.options._

import com.socialnet.users.data.users._

import java.sql.Connection

class UserDatabase (config : DBConfig)
    extends SqlDatabase (config.kind, config.addr, config.port, config.user, config.password)
{
  override def postConnection () = {
    this.createDatabaseIfNotExists ("Users")
    UserRegistry.createTable (this)
    // UserRegistry.findByLogin (this, "admin") match {
    //   case None =>
    //     UserRegistry.insert (this, User (1, "admin", BCrypt.hashpw ("admin", BCrypt.gensalt(12))))
    //   case _ => {}
    // }

    // UserRegistry.findByLogin (this, "user") match {
    //   case None =>
    //     UserRegistry.insert (this, User (2, "user", BCrypt.hashpw ("user", BCrypt.gensalt(12))))
    //   case _ => {}
    // }
  }
}
