#pragma once

#include <string>

namespace tex {

    class File {
    protected:

        // The name of the beamer file
        std::string _name;

    public:

        File (const std::string & name);

        /**
         * Write the file to an output file
         * @params:
         *    - outDir: the directory in which the export is made
         *    - compile: if true run the latex command to export it to pdf file
         */
        virtual void write (const std::string & outDir) = 0;


        virtual ~File () = 0;

        
    };

}
