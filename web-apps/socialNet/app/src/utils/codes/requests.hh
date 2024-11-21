#pragma once

#include <cstdint>

namespace socialNet::utils {

  enum RequestCode : int64_t {
    NONE = 0,
    CHECK_CONNECTED,
    CLOSE_SERVICE,
    CREATE,
    CTOR_MESSAGE,
    FIND,
    FIND_BY_ID,
    FIND_BY_LOGIN,
    FOLLOWERS,
    FOLLOWERS_LEN,
    HOME_TIMELINE,
    HOME_TIMELINE_LEN,
    IS_FOLLOWING,
    LOGIN_USER,
    READ_POST,
    REGISTER_SERVICE,
    REGISTER_USER,
    STORE,
    SUBMIT_POST,
    SUBSCIRPTIONS,
    SUBSCRIBE,
    SUBS_LEN,
    UNSUBSCRIBE,
    UPDATE_TIMELINE,
    USER_TIMELINE,
    USER_TIMELINE_LEN,
    POISON_PILL
  };

}
