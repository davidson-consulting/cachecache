#pragma once

#include <rd_utils/foreign/CLI11.hh>
#include <rd_utils/concurrency/timer.hh>
#include <rd_utils/utils/_.hh>
#include <rd_utils/concurrency/thread.hh>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "cachecache.hh"
#include <service/clock/clock.hh>

namespace cachecache {
    // will manage first get, gets, set, add, replace, append, delete
    enum class OPERATION {
        GET
        ,GETS // get with CAS, not implemented in redis
        ,SET
        ,ADD // set only if key does not exists, SET with NX option in redis
        ,REPLACE // set + XX option in redis
        ,CAS // compare and swap, not implemented in redis
        ,APPEND
        ,PREPEND
        ,DELETE
        ,INCR
        ,DECR
    };

    const std::unordered_map<std::string, OPERATION> STR_TO_OPERATION = {
        {"get", OPERATION::GET}
        , {"gets", OPERATION::GETS}
        , {"set", OPERATION::SET}
        , {"add", OPERATION::ADD}
        , {"replace", OPERATION::REPLACE}
        , {"cas", OPERATION::CAS}
        , {"append", OPERATION::APPEND}
        , {"prepend", OPERATION::PREPEND}
        , {"delete", OPERATION::DELETE}
        , {"incr", OPERATION::INCR}
        , {"decr", OPERATION::DECR}
    };

    struct line {
        std::string timestamp;
        std::string key;
        int keysize;
        int valuesize;
        int clientid;
        OPERATION operation;
        int TTL;
    };

    //extern rd_utils::concurrency::signal<> exitSignal;

    class Generator {
    public:
        Generator();
        ~Generator();

        Generator(Generator&) = delete;
        void operator=(Generator&) = delete;

        Generator(Generator&&);
        void operator=(Generator&&);

        void configure(const std::string & traces, int nb_seconds, int frequency, Cachecache* target, Clock* clock, std::shared_ptr<bool> finished);
        void run(rd_utils::concurrency::Thread);
        void run();

    private:
        rd_utils::concurrency::timer _timer;
        float _target_time = 1;

        std::string _traces;
        int _nb_seconds; 
        Cachecache* _target;
        Clock* _clock;

        std::vector<rd_utils::concurrency::Thread> _threads;

        std::shared_ptr<bool> _finished;
        bool _stop = false;

        // metrics
        int _ignored_lines = 0;
        int _time = 0;

        void process(std::string&);
        line parseLine(const std::string &) const;
        void dispose();
    
    };
}
