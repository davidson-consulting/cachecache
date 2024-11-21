package com.socialnet.timeline.database

import com.socialnet.utils.database._
import com.socialnet.master.options._

import com.socialnet.timeline.data.user._
import com.socialnet.timeline.data.home._

import java.sql.Connection

class TimelineDatabase (config : DBConfig)
    extends SqlDatabase (config.kind, config.addr, config.port, config.user, config.password)
{
  override def postConnection () = {
    this.createDatabaseIfNotExists ("Timeline")
    UserTimelineRegistry.createTable (this)
    HomeTimelineRegistry.createTable (this)
  }
}
