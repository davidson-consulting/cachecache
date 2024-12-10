#include "key.hh"
#include "utils/csts.hh"

#include <string.h>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace kv_store::memory {

    Key::Key ()
    : _data (nullptr)
    , _length (0)
    {}

    Key::Key (uint32_t len)
        : _data (nullptr)
        , _length (0)
    {
        if (MAX_KEY_SIZE < MemorySize::B (len)) {
            throw std::runtime_error ("Key too big");
        }

        uint8_t * mem = reinterpret_cast <uint8_t*> (malloc (len));
        if (mem == nullptr) {
            throw std::runtime_error ("Out of memory");
        }

        this-> _data = mem;
        this-> _length = len;
    }

    
    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          SETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Key::set (const std::string & content) {
        if (MAX_KEY_SIZE < MemorySize::B (content.length ())) {
            throw std::runtime_error ("Key is too big");
        }

        if (this-> _length != content.length ()) {
            if (this-> _data != nullptr) {
                ::free (this-> _data);
            }

            this-> _data = reinterpret_cast <uint8_t*> (malloc (content.length ()));
            if (this-> _data == nullptr) {
                throw std::runtime_error ("Out of memory");
            }

            this-> _length = content.length ();
        }

        ::memcpy (this-> _data, content.data (), content.length ());
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint64_t Key::hash () const {
        uint64_t p = 31;
        uint64_t m = 1000000009;
        uint64_t res = 0;
        uint64_t p_pow = 1;

        for (uint32_t i = 0 ; i < this-> _length ; i++) {
            res = ((res + static_cast <uint64_t> (this-> _data [i]) + 1) * p_pow) % m;
            p_pow = (p_pow * p) % m;
        }

        return res;
    }

    uint8_t* Key::data () {
        return this-> _data;
    }

    const uint8_t* Key::data () const {
        return this-> _data;
    }

    uint32_t Key::len () const {
        return this-> _length;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::ostream& operator<< (std::ostream & s, const Key & k) {
        if (k.len () == 0) {
            s << "#null";
        } else {
            for (uint32_t i = 0 ; i < k.len () ; i++) {
                s << k.data ()[i];
            }
            s << "(" << (k.hash () % kv_store::memory::NB_KVMAP_SLAB_ENTRIES) << ")";
        }

        return s;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ===================================          DISPOSING          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    Key::~Key () {
        if (this-> _data != nullptr) {
            ::free (this-> _data);
            this-> _data = nullptr;
            this-> _length = 0;
        }
    }



}
