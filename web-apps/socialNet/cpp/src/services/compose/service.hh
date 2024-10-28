#pragma once


#include <rd_utils/_.hh>
#include <utils/codes/response.hh>

namespace socialNet::compose {

  class ComposeService : public rd_utils::concurrency::actor::ActorBase {
  private:

    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;

    std::string _iface;

    std::string _secret;

    std::string _issuer;

  public:

    ComposeService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::ConfigNode & conf);

    void onStream (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream);

    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onQuit () override;

  private:

    std::shared_ptr <rd_utils::utils::config::ConfigNode> submitNewPost (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr <rd_utils::utils::config::ConfigNode> login (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr <rd_utils::utils::config::ConfigNode> registerUser (const rd_utils::utils::config::ConfigNode & msg);

    bool checkConnected (const rd_utils::utils::config::ConfigNode & node);
    std::string createJWTToken (uint32_t uid);

    void streamTimeline (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream, const std::string & kind);
    void streamSubscriptions (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream, const std::string & kind);

  };

}
