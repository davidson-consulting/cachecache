#include "client.hh"
#include <rd_utils/_.hh>

namespace socialNet::utils {

  CacheClient::CacheClient (const std::string & addr, uint32_t port) :
    _addr (rd_utils::net::SockAddrV4 (addr, port))
  {}

  bool CacheClient::get (const std::string & key, std::string & value) {
    try {
      auto session = this-> stream ();
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
      auto session = this-> stream ();
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

  bool CacheClient::get (const std::string & key, std::vector <uint32_t> & result) {
    try {
      auto session = this-> stream ();
      session-> sendI32 ('g');
      session-> sendU32 (key.length ());
      session-> sendStr (key);

      auto resLen = session-> receiveU32 ();
      if (resLen == 0 || resLen > 1024 * 1024 * 4) return false;
      result.resize (resLen / sizeof (uint32_t));

      return session-> receiveRaw (reinterpret_cast <uint8_t*> (result.data ()), resLen);
    } catch (...) {
      return false;
    }
  }


  bool CacheClient::set (const std::string & key, const uint8_t* value, uint32_t size) {
    try {
      auto session = this-> stream ();
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

  std::shared_ptr <rd_utils::net::TcpStream> CacheClient::stream () {
    if (this-> _stream == nullptr || !this-> _stream-> isOpen ()) {
      auto s = std::make_shared <rd_utils::net::TcpStream> (this-> _addr);
      s-> setSendTimeout (5.f);
      s-> setRecvTimeout (5.f);

      try {
        s-> connect ();
      } catch (const std::runtime_error & err) { // failed to connect
        LOG_INFO ("THROW 9");
        throw std::runtime_error ("Failed to connect");
      }

      this-> _stream = s;
      return s;
    }

    return this-> _stream;
  }

}
