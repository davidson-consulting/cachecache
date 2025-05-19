#pragma once

#include "base.hh"
#include <vector>
#include <string>
#include <memory>
#include <sstream>

namespace tex {

    class TableFigure : public Figure {

        std::vector <std::string> _head;
        std::vector <std::vector <std::string> > _rows;

        double _size = -1;
        std::string _caption;

    public:

        /**
         * A table
         */
        TableFigure (const std::string & name, const std::vector <std::string> & head);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GET/SET          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        TableFigure& caption (const std::string & cap);
        TableFigure& resize (double size);

        /**
         * Add rows to the table
         */
        TableFigure& addRow (const std::vector <std::string> & values);


        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          TOSTREAM          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Write the table to the stream
         * @params:
         *    - the stream to fill
         */
        void toStream (std::stringstream & stream) const override;



    };

}
