#include "counter.hh"

#include <rd_utils/_.hh>

namespace socialNet::utils {

    Counter::Counter (const std::string & name, uint32_t trigger)
        : _time ()
        , _timeSum (0)
        , _nb (0)
        , _trigger (trigger)
        , _name (name)
    {
        rd_utils::utils::create_directory ("./.counters/", true);
    }

    void Counter::insert (double time) {
        WITH_LOCK (this-> _m) {
            this-> _timeSum += time;
            this-> _time.push_back (time);
            this-> _nb += 1;
            if (this-> _nb % this-> _trigger == 0) {
                LOG_INFO ("Counter : ", this-> _name, " => ", this-> _timeSum / this-> _nb, "(", this-> _timeSum, "/", this-> _nb, ")");
                this-> exportTimes ();
                this-> _time.clear ();
            }
        }
    }

    void Counter::exportTimes () {
        std::ofstream f (std::string ("./.counters/") + this-> _name, std::ios::app);
        for (auto & it : this-> _time) {
            f << std::to_string (it) << std::endl;
        }
        f.close ();
    }

}
