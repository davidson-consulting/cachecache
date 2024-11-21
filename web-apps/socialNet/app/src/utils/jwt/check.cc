#include <jwt-cpp/jwt.h>
#include "check.hh"


namespace socialNet::utils {

  bool checkConnected (const rd_utils::utils::config::ConfigNode & msg, const std::string & issuer, const std::string & secret) {
    try {
      auto jwt_tok = jwt::decode (msg ["jwt_token"].getStr ());
      auto uid = msg ["userId"].getI ();

      auto verif = jwt::verify ()
        .with_issuer (issuer)
        .with_claim ("userId", jwt::claim (std::to_string (uid)))
        .allow_algorithm (jwt::algorithm::hs256 {secret});

      verif.verify (jwt_tok);

      return true;
    } catch (...) {}

    return false;
  }

  std::string createJWTToken (uint32_t uid, const std::string & issuer, const std::string & secret) {
    return jwt::create ()
      .set_type ("JWS")
      .set_issuer (issuer)
      .set_payload_claim ("userId", jwt::claim (std::to_string (uid)))
      .sign (jwt::algorithm::hs256 {secret});
  }

}
