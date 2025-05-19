#include "plot.hh"
#include <iostream>
#include "../../analyser/utils.hh"

namespace tex {

    Plot::Plot () {}

    Plot& Plot::append (double value) {
        this-> _values.push_back (value);
        return *this;
    }

    Plot& Plot::name (const std::string & name) {
        this-> _name = name;
        return *this;
    }

    Plot& Plot::factor (uint32_t factor) {
        this-> _factor = factor;
        return *this;
    }

    Plot& Plot::smooth (uint32_t factor) {
        this-> _smooth = factor;
        return *this;
    }

    Plot& Plot::minIndex (uint32_t min) {
        this-> _minIndex = min;
        return *this;
    }

    Plot& Plot::lineWidth (double w) {
        this-> _lineWidth = w;
        return *this;
    }

    Plot& Plot::color (const std::string & color) {
        this-> _color = color;
        return *this;
    }

    Plot& Plot::style (const std::string & style) {
        this-> _kind = style;
        return *this;
    }

    Plot& Plot::legend (const std::string & legend) {
        this-> _legend = legend;
        return *this;
    }

    const std::string & Plot::getLegend () const {
        return this-> _legend;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          TOSTREAM          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Plot::toStream (std::stringstream & ss) const {
        ss << "\t\t\\addplot " << this-> plotConfig () << " coordinates {" << std::endl;
        uint32_t i = 0;
        if (this-> _smooth == 0) {
            for (auto & it : this-> _values) {
                ss << "\t\t\t (" << this-> _minIndex + (i * this-> _factor) << ", " << it << ")" << std::endl;
                i += 1;
            }
        } else {
            auto res = analyser::smooth (this-> _values, this-> _smooth);
            for (auto & it : res) {
                ss << "\t\t\t (" << this-> _minIndex + (i * this-> _smooth) << ", " << it << ")" << std::endl;
                i += 1;
            }
        }
        ss << "\t\t};" << std::endl;

        if (this-> _legend != "") {
            ss << "\\addlegendentry{" << this-> _legend << "};" << std::endl;
        }
    }

    std::string Plot::plotConfig () const {
        std::stringstream result;
        bool fst = true;
        result << " [";
        if (this-> _kind != "") { result << this-> _kind << ", "; fst = false; }
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

        if (this-> _name != "") {
            if (!fst) result << ", ";
            result << "name path=" << this-> _name;
            fst = false;
        }

        result << "]";
        if (!fst) return result.str ();

        else return "";
    }

}
