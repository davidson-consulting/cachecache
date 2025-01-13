#include <iostream>

#include "memory/ram/meta.hh"
#include <string.h>

using namespace kv_store::memory;

#define NB_KEYS 1000

auto main(int argc, char *argv[]) -> int {
    MetaRamCollection coll (10);

    std::string c ("Content");
    Value v (c.length ());
    ::memcpy (v.data (), c.c_str (), v.len ());

    for (uint32_t i = 0 ; i < NB_KEYS; i++) {
        Key k;
        k.set ("Hello " + std::to_string (i));

        std::cout << "Insert : " << k << " => " << v << std::endl;
        coll.insert (k, v);

        auto fnd = coll.find (k);
        if (fnd != nullptr) {
            std::cout << *fnd << std::endl;
        }
    }

    std::cout << coll << std::endl;
    for (uint32_t i = 0 ; i < NB_KEYS ; i++) {
        Key k;
        k.set ("Hello " + std::to_string (i));
        coll.remove (k);
    }

    std::cout << coll << std::endl;

    return 0;
}
