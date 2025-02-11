#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <sstream>

namespace tex {

    class IndexedPlot {
    private:

        // The values of the plot
        std::vector <std::pair <double, double> > _values;

        // The name of the plot (for the legend)
        std::string _legend;

        // The plot kind
        std::string _kind = "";

        // The width of the line in mm
        double _lineWidth = -1;

        // The color of the plot
        std::string _color = "";

    public:

        /**
         * Create a plot
         * @params:
         *     - name: the name of the plot for the legend
         *     - minIndex: the min index (in the list of points)
         */
        IndexedPlot ();

        /**
         * Append a value to the plot
         */
        IndexedPlot& append (double x, double y);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GET/SET          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        IndexedPlot& lineWidth (double w);
        IndexedPlot& color (const std::string & color);
        IndexedPlot& style (const std::string & style);
        IndexedPlot& legend (const std::string & legend);


        const std::string & getLegend () const;

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          TOSTREAM          ====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Write the plot to the stream
         * @params:
         *    - the stream to fill
         */
        void toStream (std::stringstream & stream) const;

        /**
         * @returns: the tikz format of the plot configuration
         */
        std::string plotConfig () const;


    };




}
