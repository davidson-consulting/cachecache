#pragma once

#include <rd_utils/_.hh>
#include <utils/codes/response.hh>
#include <utils/codes/requests.hh>
#include "database.hh"

namespace socialNet::short_url {

  class ShortUrlService : public rd_utils::concurrency::actor::ActorBase {
  private:

    std::shared_ptr <rd_utils::concurrency::actor::ActorRef> _registry;

    std::string _iface;

    ShortUrlDatabase _db;

  public:

    /**
     * @params:
     *    - name: the name of the actor
     *    - sys: the system responsible of the actor
     *    - configFile: the configuration file used to configure db access and other stuff
     */
    ShortUrlService (const std::string & name, rd_utils::concurrency::actor::ActorSystem * sys, const rd_utils::utils::config::Dict & conf);

    /**
     * Request for insertions
     */
    std::shared_ptr<rd_utils::utils::config::ConfigNode> onRequest (const rd_utils::utils::config::ConfigNode & msg);

    void onStream (const rd_utils::utils::config::ConfigNode & msg, rd_utils::concurrency::actor::ActorStream & stream) override;

    void onMessage (const rd_utils::utils::config::ConfigNode& msg);

    void onQuit () override;

  private :

    /**
     * Read one shorturl
     */
    std::shared_ptr <rd_utils::utils::config::ConfigNode> readOrGenerate (const rd_utils::utils::config::ConfigNode & msg);

    std::shared_ptr <rd_utils::utils::config::ConfigNode> find (const rd_utils::utils::config::ConfigNode & msg);

    /**
     * Create a short url
     */
    rd_utils::memory::cache::collection::FlatString<16> createShort ();

    void streamCreate (rd_utils::concurrency::actor::ActorStream &);

  };



}
