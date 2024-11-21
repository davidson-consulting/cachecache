package com.socialnet.short_url.cache

import com.socialnet.utils.database._
import com.socialnet.master.options._
import com.socialnet.utils.cache._

class ShortUrlCache (config : CacheConfig)
    extends RedisCache (config.addr, config.port)
{
  override def postConnection () = {}
}
