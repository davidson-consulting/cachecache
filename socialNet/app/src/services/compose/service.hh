#pragma once


#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>
#include <rd_utils/_.hh>


namespace socialNet::compose {

  class ComposeService : public rd_utils::concurrency::actor::ActorBase {
  private:

    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;
    bool _needClose = true;

    rd_utils::utils::config::Dict _config;
    uint32_t _uid;
    std::string _iface;

    std::string _secret;

    std::string _issuer;

  public:

    ComposeService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf);

    void onStart () override;

    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onMessage (const rd_utils::utils::config::ConfigNode & msg);

    void onQuit () override;

  private:

    std::shared_ptr <rd_utils::utils::config::ConfigNode> submitNewPost (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr <rd_utils::utils::config::ConfigNode> login (const rd_utils::utils::config::ConfigNode & msg);
    std::shared_ptr <rd_utils::utils::config::ConfigNode> registerUser (const rd_utils::utils::config::ConfigNode & msg);

    bool checkConnected (const rd_utils::utils::config::ConfigNode & node);
    std::string createJWTToken (uint32_t uid);

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          STREAMING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

      std::shared_ptr <rd_utils::utils::config::ConfigNode> readTimeline (const rd_utils::utils::config::ConfigNode & msg, utils::RequestCode kind);
      std::shared_ptr <rd_utils::utils::config::ConfigNode> readSubscriptions (const rd_utils::utils::config::ConfigNode & msg, utils::RequestCode kind);

      std::shared_ptr <rd_utils::utils::config::ConfigNode> retreiveTimeline (uint32_t uid, uint32_t page, uint32_t nb, utils::RequestCode kind);
      std::shared_ptr <rd_utils::utils::config::ConfigNode> retreiveFollSubs (uint32_t uid, uint32_t page, uint32_t nb, utils::RequestCode kind);

  };

}
