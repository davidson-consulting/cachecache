#pragma once

#include <sstream>
#include <cstdint>

namespace tex {

    class Figure {
    protected:

        // The name of the figure
        std::string _name;

    public:

        Figure (const std::string & name);

        /**
         * Write the figure to the stream
         * @params:
         *    - the stream to fill
         */
        virtual void toStream (std::stringstream & stream) const = 0;

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GETTERS          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * @returns: the name of the figure
         */
        const std::string & getName () const;



        virtual ~Figure () = 0;
    };



}
