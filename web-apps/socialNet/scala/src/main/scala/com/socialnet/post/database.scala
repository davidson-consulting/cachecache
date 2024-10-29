package com.socialnet.post.database

import com.socialnet.utils.database._
import com.socialnet.master.options._

import com.socialnet.post.data.post._
import com.socialnet.post.data.tags._

import java.sql.Connection

class PostDatabase (config : DBConfig)
    extends SqlDatabase (config.kind, config.addr, config.port, config.user, config.password)
{
  override def postConnection () = {
    this.createDatabaseIfNotExists ("Post")
    PostRegistry.createTable (this)
    UserTagsRegistry.createTable (this)
  }
}
