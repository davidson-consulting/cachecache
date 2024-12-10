#include <iostream>

#include "memory/slab.hh"
#include <string.h>

using namespace kv_store::memory;

#define NB_KEYS 10

auto main(int argc, char *argv[]) -> int {
    KVMapSlab slab (0);

    std::string c ("Content");
    Value v (c.length ());
    ::memcpy (v.data (), c.c_str (), v.len ());

    for (uint32_t i = 0 ; i < NB_KEYS; i++) {

        Key k;
        k.set ("Hello " + std::to_string (i));


        std::cout << "Insert : " << k << " => " << v << std::endl;
        slab.alloc (k, v);

        auto fnd = slab.find (k);
        if (fnd != nullptr) {
            std::cout << *fnd << std::endl;
        }
    }

    std::cout << slab << std::endl;
    for (uint32_t i = NB_KEYS/2 ; i < NB_KEYS ; i++) {
        Key k;
        k.set ("Hello " + std::to_string (i));
        slab.remove (k);
    }

    std::cout << slab << std::endl;

    return 0;
}
