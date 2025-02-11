#pragma once

#include "base.hh"
#include "plot.hh"
#include "indexedplot.hh"
#include <vector>
#include <string>
#include <memory>
#include <sstream>


namespace tex {

        /**
         * A tikz axis figure
         */
        class AxisFigure : public Figure {

                struct FillPlot {
                        std::string a;
                        std::string b;
                        std::string color;
                };

        private:

                // The list of plots in the figure
                std::vector <std::shared_ptr <Plot> > _plots;
                std::vector <std::shared_ptr <IndexedPlot> > _indexPlots;

                std::vector <FillPlot> _fillPlots;

                // The xlabel
                std::string _xlabel;

                // The ylabel
                std::string _ylabel;

                // The caption of the figure
                std::string _caption;

                bool _smooth = false;
                bool _hist = false;

                uint32_t _nbLegendPerColumn = 3;

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
                AxisFigure& hist (bool hist);
                AxisFigure& ylabel (const std::string & label);
                AxisFigure& xlabel (const std::string & label);
                AxisFigure& nbColumnLegend (uint32_t nb);
                AxisFigure& caption (const std::string & cap);

                /**
                 * Add a plot in the figure
                 */
                AxisFigure& addPlot (std::shared_ptr <Plot> plot);
                AxisFigure& addPlot (std::shared_ptr <IndexedPlot> plot);

                /**
                 * Fill color between two plots
                 */
                AxisFigure& addFillPlot (const std::string & a, const std::string & b, const std::string & color);

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
