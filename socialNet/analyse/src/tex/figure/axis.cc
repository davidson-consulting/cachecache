#include "axis.hh"


namespace tex {

    AxisFigure::AxisFigure (const std::string & name)
        : Figure (name)
    {}


    AxisFigure& AxisFigure::smooth (bool smooth) {
        this-> _smooth = smooth;
        return *this;
    }

    AxisFigure& AxisFigure::ylabel (const std::string & ylabel) {
        this-> _ylabel = ylabel;
        return *this;
    }

    AxisFigure& AxisFigure::xlabel (const std::string & xlabel) {
        this-> _xlabel = xlabel;
        return *this;
    }

    AxisFigure& AxisFigure::caption (const std::string & caption) {
        this-> _caption = caption;
        return *this;
    }

    AxisFigure& AxisFigure::addPlot (std::shared_ptr <Plot> plt) {
        this-> _plots.push_back (plt);
        return *this;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          TOSTREAM          ====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void AxisFigure::toStream (std::stringstream & ss) const {
        ss << "\\begin{figure}[h]" << std::endl;
        ss << "    \\centering" << std::endl;
        ss << "    \\begin{adjustbox}{max width=1.0\\linewidth, keepaspectratio}" << std::endl;
        ss << "        \\begin{tikzpicture}" << std::endl;
        ss << "            \\begin{axis}" << this-> axisConfig () << std::endl;

        for (auto & it : this-> _plots) {
            it-> toStream (ss);
        }

        ss << "           \\end{axis}" << std::endl;
        ss << "      \\end{tikzpicture}" << std::endl;
        ss << "  \\end{adjustbox}" << std::endl;
        if (this-> _caption != "") {
            ss << "  \\caption{" << this-> _caption << "}" << std::endl;
        }

         ss << "\\end{figure}" << std::endl;
    }

    std::string AxisFigure::axisConfig () const {
        std::stringstream ss;
        bool fst = true;
        ss << "[";
        if (this-> _ylabel != "") { ss << "ylabel = " << this-> _ylabel; fst = false; }
        if (this-> _xlabel != "") {
            if (!fst) ss << ", " << std::endl;
            ss << "xlabel = " << this-> _xlabel; fst = false;
        }

        if (!fst) ss << ", " << std::endl;
        ss << "legend style = {nodes={scale=0.9, transform shape}, at={(0.03,1.2)}, anchor=north west, draw=black, fill=white, align=left, legend columns=3}," << std::endl;

        if (this-> _smooth) ss << "smooth,";
        ss << "mark size = 0pt,\n cycle list name = exotic,\n  axis lines* = left]";
        return ss.str ();
    }



}
