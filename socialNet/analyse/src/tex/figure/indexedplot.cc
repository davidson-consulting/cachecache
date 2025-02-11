#include "indexedplot.hh"
#include <algorithm>

namespace tex {

    IndexedPlot::IndexedPlot () {}

    IndexedPlot& IndexedPlot::append (double x, double y) {
        this-> _values.push_back ({x, y});
        return *this;
    }

    IndexedPlot& IndexedPlot::lineWidth (double w) {
        this-> _lineWidth = w;
        return *this;
    }

    IndexedPlot& IndexedPlot::color (const std::string & color) {
        this-> _color = color;
        return *this;
    }

    IndexedPlot& IndexedPlot::style (const std::string & style) {
        this-> _kind = style;
        return *this;
    }

    IndexedPlot& IndexedPlot::legend (const std::string & legend) {
        this-> _legend = legend;
        return *this;
    }

    const std::string & IndexedPlot::getLegend () const {
        return this-> _legend;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          TOSTREAM          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void IndexedPlot::toStream (std::stringstream & ss) const {
        ss << "\t\t\\addplot " << this-> plotConfig () << " coordinates {" << std::endl;
        uint32_t i = 0;
        for (auto & it : this-> _values) {
            ss << "\t\t\t (" << it.first << ", " << it.second << ")" << std::endl;
            i += 1;
        }
        ss << "\t\t};" << std::endl;

        if (this-> _legend != "") {
            ss << "\\addlegendentry{" << this-> _legend << "};" << std::endl;
        }
    }

    std::string IndexedPlot::plotConfig () const {
        std::stringstream result;
        bool fst = true;
        result << "+[";
        if (this-> _kind != "") { result << this-> _kind; fst = false; }
        if (this-> _lineWidth != -1) {
            if (!fst)  result << ", ";
            result << "line width = " << this-> _lineWidth << "mm" << std::endl;
            fst = false;
        }

        if (this-> _color != "") {
            if (!fst) result << ", ";
            result << "color = " << this-> _color;
            fst = false;
        }

        result << "]";
        if (!fst) return result.str ();
        else return "";
    }

}
