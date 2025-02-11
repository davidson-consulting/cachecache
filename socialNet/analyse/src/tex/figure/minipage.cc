#include "minipage.hh"


namespace tex {

    MiniPageFigure::MiniPageFigure (const std::string & name)
        : Figure (name)
    {}

    MiniPageFigure& MiniPageFigure::addFigure (std::shared_ptr <Figure> fig, double width) {
        this-> _figs.push_back (fig);
        this-> _width.push_back (width);
        return *this;
    }

    void MiniPageFigure::toStream (std::stringstream & ss) const {
        for (uint32_t i = 0 ; i < this-> _figs.size () ; i++) {
            ss << "\\begin{minipage}{0." << this-> _width [i] << "\\linewidth}" << std::endl;
            this-> _figs [i]-> toStream (ss);
            ss << "\\end{minipage}\\hfill";
        }
    }

}
