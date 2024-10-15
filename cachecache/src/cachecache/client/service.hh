#pragma once

#include <rd_utils/_.hh>

namespace cachecache::client {

  class CacheClient {
  private :

    // The connection pool
    std::shared_ptr <rd_utils::net::TcpPool> _connections;

  public:

        /**
         * ========================================================
         * ========================================================
         * ====================   DELETED  ========================
         * ========================================================
         * ========================================================
         */
        CacheClient (const CacheClient &) = delete;
        void operator=(const CacheClient&) = delete;
        CacheClient (CacheClient &&) = delete;
        void operator=(CacheClient &&) = delete;

  public:

    /**
     * Create a new Cache client
     * @params:
     *    - addr: the address of the server
     *    - port: the port of the server
     *    - poolSize: the number of concurrent connection to the cache server
     */
    CacheClient (const std::string & addr, uint32_t port, uint32_t poolSize = 1);

    /**
     * Get a value from a key
     */
    std::string get (const std::string & key);

    /**
     * Set a key/value
     */
    void set (const std::string & key, const std::string & value);

  };


}
