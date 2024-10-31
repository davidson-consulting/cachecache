package com.socialnet.social_graph.database

import com.socialnet.utils.database._
import com.socialnet.master.options._

import com.socialnet.social_graph.data.sub._

import java.sql.Connection

class SocialGraphDatabase (config : DBConfig)
    extends SqlDatabase (config.kind, config.addr, config.port, config.user, config.password)
{
  override def postConnection () = {
    this.createDatabaseIfNotExists ("SocialGraph")
    SubRegistry.createTable (this)
  }
}
