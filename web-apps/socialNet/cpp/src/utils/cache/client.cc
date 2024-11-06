#include "client.hh"
#include <rd_utils/_.hh>

namespace socialNet::utils {

  CacheClient::CacheClient (const std::string & addr, uint32_t port, uint32_t poolSize) {
    this-> _connections = std::make_shared <rd_utils::net::TcpPool> (rd_utils::net::SockAddrV4 (addr, port), poolSize);
  }

  bool CacheClient::get (const std::string & key, std::string & value) {
    try {
      auto session = this-> _connections-> get ();
      session-> sendI32 ('g');
      session-> sendU32 (key.length ());
      session-> sendStr (key);

      auto resLen = session-> receiveU32 ();
      if (resLen == 0) return false;


      return session-> receiveStr (value, resLen);
    } catch (...) {
      return false;
    }
  }


  bool CacheClient::get (const std::string & key, uint8_t * res, uint32_t size) {
    try {
      auto session = this-> _connections-> get ();
      session-> sendI32 ('g');
      session-> sendU32 (key.length ());
      session-> sendStr (key);

      auto resLen = session-> receiveU32 ();
      if (resLen != size) return false;

      return session-> receiveRaw (res, size);
    } catch (...) {
      return false;
    }
  }

  bool CacheClient::set (const std::string & key, const uint8_t* value, uint32_t size) {
    try {
      auto session = this-> _connections-> get ();
      session-> sendI32 ('s');
      session-> sendU32 (key.length ());
      session-> sendStr (key);

      session-> sendU32 (size);
      session-> sendRaw (value, size);

      auto i = session-> receiveI32 ();
      return i == 1;
    } catch (...) {
      return false;
    }
  }

}
