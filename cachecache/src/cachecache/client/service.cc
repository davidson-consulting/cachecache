#include "service.hh"

namespace cachecache::client {

  CacheClient::CacheClient (const std::string & addr, uint32_t port, uint32_t poolSize) {
    this-> _connections = std::make_shared <rd_utils::net::TcpPool> (rd_utils::net::SockAddrV4 (addr, port), poolSize);
  }

  bool CacheClient::get (const std::string & key, std::string & res) {
    try {
      auto session = this-> _connections-> get ();
      session-> sendI32 ('g');
      session-> sendI32 (key.length ());
      session-> sendStr (key);

      auto resLen = session-> receiveI32 ();
      if (resLen == 0) return false;

      res = session-> receiveStr (resLen);
      return true;
    } catch (...) {
      return false;
    }
  }

  bool CacheClient::set (const std::string & key, const std::string & value) {
    try {
      auto session = this-> _connections-> get ();
      session-> sendI32 ('s');
      session-> sendI32 (key.length ());
      session-> sendStr (key);

      session-> sendI32 (value.length ());
      session-> sendStr (value);

      auto i = session-> receiveI32 ();
      return i == 1;
    } catch (...) {
      return false;
    }
  }

}
