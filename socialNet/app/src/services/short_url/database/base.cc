#include "base.hh"


namespace socialNet::short_url {

    ShortUrlDatabase::ShortUrlDatabase () :
        _cache (nullptr)
    {}

    void ShortUrlDatabase::dispose () {
        this-> _cache.reset ();
    }

    void ShortUrlDatabase::configureCache (const std::string & ch, const rd_utils::utils::config::ConfigNode & conf) {
        if (conf.contains ("cache") && ch != "") {
            auto cacheAddr = conf["cache"][ch].getOr ("addr", "localhost");
            auto cachePort = conf["cache"][ch].getOr ("port", 6650);
            this-> _cache = std::make_shared <socialNet::utils::CacheClient> (cacheAddr, cachePort);
        }
    }

    bool ShortUrlDatabase::findUrlFromShortInCache (ShortUrl & p) {
        if (this-> _cache == nullptr) return false;

        std::string sh (p.sh, strlen (p.sh));
        if (this-> _cache-> get ("urlshort/" + sh, reinterpret_cast<uint8_t*> (p.ln), LONG_LEN)) {
            return true;
        }

        return false;
    }

    bool ShortUrlDatabase::findUrlFromLongInCache (ShortUrl & p) {
        if (this-> _cache == nullptr) return false;

        std::string ln (p.ln, strlen (p.ln));
        if (this-> _cache-> get ("urllong/" + ln, reinterpret_cast<uint8_t*> (p.sh), SHORT_LEN)) {
            return true;
        }

        return false;
    }

     void ShortUrlDatabase::insertUrlInCache (ShortUrl & p) {
        if (this-> _cache == nullptr) return;

        std::string ln (p.ln, strlen (p.ln));
        std::string sh (p.sh, strlen (p.sh));

        this-> _cache-> set ("urllong/" + ln, reinterpret_cast<uint8_t*> (p.sh), SHORT_LEN);
        this-> _cache-> set ("urlshort/" + sh, reinterpret_cast<uint8_t*> (p.ln), LONG_LEN);
    }

}

std::ostream& operator<< (std::ostream & s, const socialNet::short_url::ShortUrl & p) {
  s << "ShortURL (" << p.sh << " -> " << p.ln << ")";
  return s;
}
