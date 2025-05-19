#include "table.hh"


namespace tex {

    TableFigure::TableFigure (const std::string & name, const std::vector <std::string> & head)
        : Figure (name)
        , _head (head)
    {}

    TableFigure& TableFigure::caption (const std::string & cap) {
        this-> _caption = cap;
        return *this;
    }

    TableFigure& TableFigure::resize (double size) {
        this-> _size = size;
        return *this;
    }

    TableFigure& TableFigure::addRow (const std::vector <std::string> & row) {
        this-> _rows.push_back (row);
        return *this;
    }

    void TableFigure::toStream (std::stringstream & ss) const {
         ss << "\\begin{table}[h]" << std::endl;
         if (this-> _size != -1) {
             ss << "\\begin{adjustbox}{max width=" << this-> _size << "\\width}" << std::endl;
         }

         ss << "\\begin{tabular}{|";
         for (uint32_t i = 0 ; i < this-> _head.size () ; i++) {
             ss << "c";
         }
         ss << "|}" << std::endl;
         ss << "\\hline" << std::endl;

         for (uint32_t i = 0 ; i < this-> _head.size () ; i++) {
             if (i != 0) ss << " & ";
             ss << "\\textbf{" << this-> _head [i] << "}";
         }
         ss << "\\\\ \\hline" << std::endl;

         for (auto & r : this-> _rows) {
             if (r.size () != this-> _head.size ()) {
                 ss << "\\hline" << std::endl;
             } else {
                 ss << " \\Xhline{0.005\\arrayrulewidth}" << std::endl;
                 for (uint32_t i = 0 ; i < this-> _head.size () ; i++) {
                     if (i != 0) ss << " & ";
                     ss << r [i];
                 }
                 ss << "\\\\" << std::endl;
             }
         }

         ss << "\\hline" << std::endl;
         ss << "\\end{tabular}" << std::endl;
         if (this-> _size != -1) {
             ss << "\\end{adjustbox}" << std::endl;
         }

         ss << "\\caption{" << this-> _caption << "}" << std::endl;
         ss << "\\end{table}" << std::endl;
    }

}
