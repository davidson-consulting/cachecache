#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>
#include <regex>

namespace socialNet::text {

  class TextService : public rd_utils::concurrency::actor::ActorBase {
  private:

    std::regex _userPattern;
    std::regex _urlPattern;

    uint32_t _uid;
    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;
    bool _needClose = true;

    std::string _iface;

  public:

    /**
     * @params:
     *    - name: the name of the actor
     *    - sys: the system responsible of the actor
     *    - configFile: the configuration file used to configure db access and other stuff
     */
    TextService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf);

    void onStart () override;

    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onMessage (const rd_utils::utils::config::ConfigNode&);

    void onQuit () override;

  private:

    std::shared_ptr<rd_utils::utils::config::ConfigNode> constructText (const rd_utils::utils::config::ConfigNode &);

    std::vector <std::string> findUserMentions (const std::string & msg);
    std::vector <std::string> findUrlMentions (const std::string & msg);

    std::map <std::string, std::string> transformUserTags (const std::vector <std::string> & users, uint32_t * tags, bool & succeed);
    std::map <std::string, std::string> transformUrls (const std::vector <std::string> & urls, bool & succeed);

  };
}
