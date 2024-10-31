package com.socialnet.users.cache

import com.socialnet.utils.database._
import com.socialnet.master.options._
import com.socialnet.utils.cache._

class UserCache (config : CacheConfig)
    extends RedisCache (config.addr, config.port)
{
  override def postConnection () = {}
}
