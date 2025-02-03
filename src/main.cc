#include <iostream>

#include "cache/global.hh"
#include "cache/common/_.hh"
#include <string.h>

using namespace kv_store;
using namespace kv_store::common;

#define NB_KEYS 1000

auto main(int argc, char *argv[]) -> int {
    HybridKVStore store (2);

    srand (0); // time (NULL));
    std::string c ("Content");
    Value v (c.length ());
    ::memcpy (v.data (), c.c_str (), v.len ());

    std::cout << store << std::endl;
    for (uint32_t i = 0 ; i < NB_KEYS; i++) {
        Key k;
        k.set ("Hello " + std::to_string (i));

        // std::cout << store << std::endl;
        // std::cout << "Insert : " << k << " => " << v << std::endl;
        store.insert (k, v);

        auto fnd = store.find (k);
        if (fnd != nullptr) {
            std::cout << *fnd << std::endl;
        }
    }

    Key k;
    k.set ("Hello 368");
    std::cout << store << std::endl;
    std::cout << *store.find (k) << std::endl;

    std::cout << store << std::endl;
    return 0;
}
