#pragma once

#include <rd_utils/net/_.hh>
#include <cstdint>
#include <string>

namespace socialNet::utils {

  class CacheClient {
  private:

    std::shared_ptr <rd_utils::net::TcpPool> _connections;

  public:

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          DELETED          =====================================
     * ====================================================================================================
     * ====================================================================================================
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
     * @returns:
     *    - true iif the value was found
     *    - res: the found value
     */
    bool get (const std::string & key, uint8_t * res, uint32_t size);

    /**
     * Get a value from a key
     * @returns:
     *    - true iif the value was found
     *    - res: the found value
     */
    bool get (const std::string & key, std::string & value);


    /**
     * Set a key/value
     * @returns: true iif insert succeeded
     */
    bool set (const std::string & key, const uint8_t * val, uint32_t size);

  };

}
