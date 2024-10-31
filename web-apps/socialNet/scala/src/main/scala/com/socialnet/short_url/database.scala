package com.socialnet.short_url.database

import com.socialnet.utils.database._
import com.socialnet.master.options._

import com.socialnet.short_url.data.short._

import java.sql.Connection

class ShortUrlDatabase (config : DBConfig)
    extends SqlDatabase (config.kind, config.addr, config.port, config.user, config.password)
{
  override def postConnection () = {
    this.createDatabaseIfNotExists ("ShortUrls")
    ShortUrlRegistry.createTable (this)
  }
}
