#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <sstream>

namespace tex {

        class Plot {
        private:

                std::vector <double> _values;

                // The minimal index
                uint32_t _minIndex = 0;

                // The name of the plot (for the legend)
                std::string _legend;

                // The plot kind
                std::string _kind = "";

                // The width of the line in mm
                double _lineWidth = -1;

                // The color of the plot
                std::string _color = "";

                std::string _name = "";

        public:

                /**
                 * Create a plot
                 * @params:
                 *     - name: the name of the plot for the legend
                 *     - minIndex: the min index (in the list of points)
                 */
                Plot ();

                /**
                 * Append a value to the plot
                 */
                Plot& append (double value);

                /*!
                 * ====================================================================================================
                 * ====================================================================================================
                 * ====================================          GET/SET          =====================================
                 * ====================================================================================================
                 * ====================================================================================================
                 */

                Plot& minIndex (uint32_t min);
                Plot& lineWidth (double w);
                Plot& name (const std::string & name);
                Plot& color (const std::string & color);
                Plot& style (const std::string & style);
                Plot& legend (const std::string & legend);


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
