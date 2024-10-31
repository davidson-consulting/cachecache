package com.socialnet.post.cache

import com.socialnet.utils.database._
import com.socialnet.master.options._
import com.socialnet.utils.cache._

class PostCache (config : CacheConfig)
    extends RedisCache (config.addr, config.port)
{
  override def postConnection () = {}
}
