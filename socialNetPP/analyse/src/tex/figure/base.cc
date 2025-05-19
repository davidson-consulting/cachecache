#include "base.hh"


namespace tex {

    Figure::Figure (const std::string & name)
        : _name (name)
    {}

    const std::string & Figure::getName () const {
        return this-> _name;
    }

    Figure::~Figure () {}
}
