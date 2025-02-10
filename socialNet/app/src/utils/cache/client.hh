#pragma once

#include <rd_utils/net/_.hh>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace socialNet::utils {

  class CacheClient {
  private:

    rd_utils::net::SockAddrV4 _addr;

    // The connection to cache system
    std::shared_ptr <rd_utils::net::TcpStream> _stream;

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
     */
    CacheClient (const std::string & addr, uint32_t port);

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
     * Get a value from a key
     * @returns:
     *    - true iif the value was found
     *    - res: the found value
     */
    bool get (const std::string & key, std::vector<uint32_t> & value);



    /**
     * Set a key/value
     * @returns: true iif insert succeeded
     */
    bool set (const std::string & key, const uint8_t * val, uint32_t size);

  private :

    /**
     * Open a stream to the cache
     */
    std::shared_ptr <rd_utils::net::TcpStream> stream ();


  };

}
