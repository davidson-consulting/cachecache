#include <iostream>

#include "cache/global.hh"
#include "cache/common/_.hh"
#include <string.h>
#include <memory>

using namespace kv_store;
using namespace kv_store::common;

#define NB_KEYS 2000000


void foo () {
    disk::MetaDiskCollection store (1024);

    srand (0); // time (NULL));
    std::string c ("Content_");
    Value v (c.length ());
    ::memcpy (v.data (), c.c_str (), v.len ());

    // std::cout << store << std::endl;
    for (uint32_t i = 0 ; i < NB_KEYS; i++) {
        Key k;
        k.set ("Hello " + std::to_string (i));

        // std::cout << store << std::endl;
        std::cout << "Insert : " << k << " => " << v << std::endl;
        store.insert (k, v);
        std::cout << store << std::endl;
    }
}

auto main(int argc, char *argv[]) -> int {
    // foo ();
    std::unique_ptr<HybridKVStore> store = HybridKVStore::TTLBased(6, 1024, 1);

    srand (0); // time (NULL));

    // std::cout << store << std::endl;
    for (uint32_t i = 0 ; i < NB_KEYS; i++) {
        Key k;
        k.set ("Hello " + std::to_string (i));

        std::string c ("Content_" + std::to_string (i));
        Value v (c.length ());
        ::memcpy (v.data (), c.c_str (), v.len ());

        store->insert (k, v);
        // auto fnd = store.find (k);
        // if (fnd != nullptr) {
        //     // std::cout << *fnd << std::endl;
        // }
    }

    // std::cout << store << std::endl;
    std::cout << "AF " << std::endl;

    Key k;
    k.set ("Hello 19");

    // std::cout << *store.find (k) << std::endl;

    // std::cout << store << std::endl;
    return 0;
}
