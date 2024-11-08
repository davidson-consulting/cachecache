#include "service.hh"

namespace cachecache::client {

  CacheClient::CacheClient (const std::string & addr, uint32_t port, uint32_t poolSize) {
    this-> _connections = std::make_shared <rd_utils::net::TcpPool> (rd_utils::net::SockAddrV4 (addr, port), poolSize);
  }


  bool CacheClient::get (const std::string & key, std::string & value) {
    try {
      auto session = this-> _connections-> get ();
      session-> getStream ().sendI32 ('g');
      session-> getStream ().sendU32 (key.length ());
      session-> getStream ().sendStr (key);

      auto resLen = session-> getStream ().receiveU32 ();
      if (resLen == 0) return false;


      return session-> getStream ().receiveStr (value, resLen);
    } catch (...) {
      return false;
    }
  }

  bool CacheClient::set (const std::string & key, const std::string & value) {
    try {
      auto session = this-> _connections-> get ();
      session-> getStream ().sendI32 ('s');
      session-> getStream ().sendI32 (key.length ());
      session-> getStream ().sendStr (key);

      session-> getStream ().sendI32 (value.length ());
      session-> getStream ().sendStr (value);

      auto i = session-> getStream ().receiveI32 ();
      return i == 1;
    } catch (...) {
      return false;
    }
  }

}
