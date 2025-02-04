#pragma once

#include "base.hh"
#include "../figure/base.hh"
#include <vector>
#include <map>
#include <memory>
#include <sstream>

namespace tex {

    /**
     * A latex beamer document
     * With one figure per frame
     */
    class Beamer : public File {
    private:

        // The list of figures
        std::map <std::string, std::vector <std::shared_ptr <Figure> > > _figures;

    public:

        Beamer (const std::string & name);

        /**
         * Add a figure to the beamer doc
         */
        Beamer& addFigure (const std::string & section, std::shared_ptr <Figure> fig);

        /**
         * Write the beamer to an output file
         * @params:
         *    - outDir: the directory in which the export is made
         *    - compile: if true run the latex command to export it to pdf file
         */
        void write (const std::string & outDir) override;

    private:

        void writeHeader (std::stringstream & ss, const std::string & inputDir);
        void writeInputHeader (const std::string & inputDir);

        void writeFooter (std::stringstream & ss);
        void writeFrame  (std::shared_ptr <Figure> fig, std::stringstream & ss, const std::string & figureDir);

    };

}
