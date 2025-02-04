#pragma once

#include "base.hh"
#include "plot.hh"
#include <vector>
#include <string>
#include <memory>
#include <sstream>


namespace tex {

    /**
     * A tikz axis figure
     */
    class AxisFigure : public Figure {
    private:

        // The list of plots in the figure
        std::vector <std::shared_ptr <Plot> > _plots;

        // The xlabel
        std::string _xlabel;

        // The ylabel
        std::string _ylabel;

        // The caption of the figure
        std::string _caption;

        bool _smooth = false;

    public:

        /**
         * A tikz figure
         */
        AxisFigure (const std::string & name);


        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GET/SET          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        AxisFigure& smooth (bool smooth);
        AxisFigure& ylabel (const std::string & label);
        AxisFigure& xlabel (const std::string & label);
        AxisFigure& caption (const std::string & cap);

        /**
         * Add a plot in the figure
         */
        AxisFigure& addPlot (std::shared_ptr <Plot> plot);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          TOSTREAM          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Write the figure to the stream
         * @params:
         *    - the stream to fill
         *    - indent: the current indentation
         */
        void toStream (std::stringstream & stream) const override;

        /**
         * @returns: the tikz format of the axis configuration
         */
        std::string axisConfig () const;

    };

}
