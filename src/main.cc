#include <iostream>

#include "cache/ram/meta.hh"
#include "cache/disk/slab.hh"
#include "cache/common/_.hh"
#include <string.h>

using namespace kv_store::disk;
using namespace kv_store::memory;
using namespace kv_store::common;

#define NB_KEYS 100

auto main(int argc, char *argv[]) -> int {
    KVMapDiskSlab coll;
    srand (time (NULL));
    std::string c ("Content");
    Value v (c.length ());
    ::memcpy (v.data (), c.c_str (), v.len ());

    std::cout << coll << std::endl;
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
    while (coll.len () > 0) {
        Key k;
        k.set ("Hello " + std::to_string (coll.len () - 1));
        if (coll.remove (k)) {
            std::cout << coll << std::endl;
        }
    }

    std::cout << coll << std::endl;

    return 0;
}
