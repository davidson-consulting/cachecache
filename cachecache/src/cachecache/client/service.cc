#include "service.hh"

namespace cachecache::client {

  CacheClient::CacheClient (const std::string & addr, uint32_t port, uint32_t poolSize) {
    this-> _connections = std::make_shared <rd_utils::net::TcpPool> (rd_utils::net::SockAddrV4 (addr, port), poolSize);
  }

  bool CacheClient::get (const std::string & key, std::string & res) {
    auto session = this-> _connections-> get ();
    session-> sendI32 ('g');
    session-> sendI32 (key.length ());
    session-> send (key.c_str (), key.length ());

    auto resLen = session-> receiveI32 ();
    if (resLen == 0) return false;

    std::stringstream ss;
    char buffer [1024];
    while (resLen > 0) {
      uint32_t rest = std::min (1024, resLen);
      if (!session-> receiveRaw<char> (buffer, rest)) {
        return "";
      }

      ss << buffer;
      resLen -= rest;
    }

    res = ss.str ();
    return true;
  }

  bool CacheClient::set (const std::string & key, const std::string & value) {
    auto session = this-> _connections-> get ();
    session-> sendI32 ('s');
    session-> sendI32 (key.length ());
    session-> send (key.c_str (), key.length ());

    session-> sendI32 (value.length ());
    session-> send (value.c_str (), value.length ());

    auto i = session-> receiveI32 ();
    return (session-> isOpen () && i == 1);

    return false;
  }

}
