#include "value.hh"
#include "csts.hh"

#include <string.h>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace kv_store::common {

    Value::Value ()
        : _data (nullptr),
          _length (0)
    {}

    Value::Value (uint32_t len)
        : _data (nullptr)
        , _length (0)
    {
        if (MAX_VALUE_SIZE < MemorySize::B (len)) {
            throw std::runtime_error ("Value is too big " + std::to_string (len) + " > " + std::to_string (MAX_VALUE_SIZE.bytes ()));
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


    void Value::set (const std::string & content) {
        if (MAX_VALUE_SIZE < MemorySize::B (content.length ())) {
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

    std::string Value::asString () const {
        return std::string (this-> _data, this-> _data + this-> _length);
    }

    const uint8_t* Value::data () const {
        return this-> _data;
    }

    uint32_t Value::len () const {
        return this-> _length;
    }

    uint8_t* Value::data () {
        return this-> _data;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ======================================          MISC          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    std::ostream& operator<< (std::ostream & s, const Value & k) {
        if (k.len () == 0) {
            s << "#null";
        } else {
            s << "[";
            for (uint32_t i = 0 ; i < k.len () ; i++) {
                s << k.data ()[i];
            }
            s << "]";
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

    Value::~Value () {
        if (this-> _data != nullptr) {
            free (this-> _data);
            this-> _data = nullptr;
            this-> _length = 0;
        }
    }

}
