#include "beamer.hh"
#include <rd_utils/_.hh>

using namespace rd_utils;
using namespace rd_utils::utils;

namespace tex {

    Beamer::Beamer (const std::string & name)
        : File (name)
    {}

    Beamer& Beamer::addFigure (const std::string & name, std::shared_ptr <Figure> fig) {
        this-> _figures [name].push_back (fig);
        return *this;
    }

    void Beamer::write (const std::string & outDir) {
        utils::create_directory (outDir, true);
        utils::create_directory (utils::join_path (outDir, this-> _name + "_input"), true);
        utils::create_directory (utils::join_path (outDir, this-> _name + "_figures"), true);
        std::stringstream result;

        this-> writeHeader (result, utils::join_path (outDir, this-> _name + "_input"));
        for (auto & sec : this-> _figures) {
            result << "\\section {" << sec.first << "}" << std::endl;
            for (auto & it : sec.second) {
                this-> writeFrame (it, result, utils::join_path (outDir, this-> _name + "_figures"));
            }
        }

        this-> writeFooter (result);
        utils::write_file (utils::join_path (outDir, this-> _name + ".tex"), result.str ());
    }

    void Beamer::writeHeader (std::stringstream & ss, const std::string & inputDir) {
        this-> writeInputHeader (inputDir);

        ss << "\\documentclass[10pt, aspectratio=1610]{beamer}" << std::endl;
        ss << "\\input{" + this-> _name + "_input/header}" << std::endl;
        ss << std::endl;
        ss << "\\begin{document}" << std::endl;
    }

    void Beamer::writeFrame (std::shared_ptr <Figure> fig, std::stringstream & ss, const std::string & figDir) {
        std::stringstream figure;
        fig-> toStream (figure);

        utils::write_file (utils::join_path (figDir, fig-> getName () + ".tex"), figure.str ());
        ss << std::endl;
        ss << "\\begin{frame}[fragile=singleslide]" << std::endl;
        ss << "    \\input{" << this-> _name << "_figures/" << fig-> getName () << "}" << std::endl;
        ss << "\\end{frame}" << std::endl;
        ss << std::endl;
    }

    void Beamer::writeFooter (std::stringstream & ss) {
        ss << "\\end{document}" << std::endl;
    }

    void Beamer::writeInputHeader (const std::string & inputDir) {
        utils::create_directory (inputDir, true);
        std::stringstream input;
        input << "\\usetheme[progressbar=frametitle]{metropolis}" << std::endl;
        input << "\\usepackage{appendixnumberbeamer}" << std::endl;

        input << "\\usetikzlibrary{calc}" << std::endl;
        input << "\\usepackage{booktabs}" << std::endl;
        input << "\\usepackage[scale=2]{ccicons}" << std::endl;
        input << "\\usepackage{pgfplots}" << std::endl;
        input << "\\usepgfplotslibrary{dateplot}" << std::endl;
        input << "\\usepackage{caption}" << std::endl;
        input << "\\captionsetup{font=scriptsize,labelfont=scriptsize}" << std::endl;
        input << "\\usepackage{xspace}" << std::endl;
        input << "\\newcommand{\\themename}{\\textbf{\\textsc{metropolis}}\\xspace}" << std::endl;
        input << "\\metroset{block=fill}" << std::endl;

        input << "\\usepackage{tikz}" << std::endl;
        input << "\\usetikzlibrary{patterns,patterns.meta}" << std::endl;
        input << "\\usepackage{pgfplots}" << std::endl;
        input << "\\pgfplotsset{compat=1.8}" << std::endl;
        input << "\\usepgfplotslibrary{statistics}" << std::endl;
        input << "\\usepgfplotslibrary{fillbetween}" << std::endl;
        input << "\\usepackage{mathtools, amssymb, amsmath, relsize}" << std::endl;
        input << "\\usepackage{multirow}" << std::endl;
        input << "\\usepackage{adjustbox}" << std::endl;
        input << "\\usepackage{xcolor}" << std::endl;
        input << "\\usepackage{colortbl}" << std::endl;
        input << "\\usepackage{graphicx}" << std::endl;

        utils::write_file (utils::join_path (inputDir, "header.tex"), input.str ());
    }


}
