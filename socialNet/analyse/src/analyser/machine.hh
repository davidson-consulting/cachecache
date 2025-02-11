#pragma once

#include <string>
#include <rd_utils/_.hh>
#include <vector>
#include <map>
#include "../tex/_.hh"

namespace analyser {

    struct EnergyTrace {
        float pdu;
        float cpu;
        float ram;
    };

    struct UsageTrace {
        double cpu;
        rd_utils::utils::MemorySize mem_anon;
        rd_utils::utils::MemorySize mem_file;
    };

    /**
     * A machine used in a run
     */
    class Machine {
    private :

        // The name of the machine
        std::string _name;

    private:

        // The global usage of the machine (sum of all the cgroups)
        std::vector <UsageTrace> _globalUsage;

        // The traces of each cgroups
        std::vector <std::map <std::string, UsageTrace> > _usage;

        // The list of cgroup executed on the machine
        std::set <std::string> _groups;

        // The energy traces of the machine
        std::vector <EnergyTrace> _energy;

        // The timestamp of the first point in traces
        uint64_t _minTimestamp;

    public:

        Machine ();

        /**
         * Configure the machine
         */
        void configure (const std::string & traceDir, const rd_utils::utils::config::ConfigNode & cfg);

        /**
         * Execute the analyze on the machine
         * @params:
         *    - outDir: the directory in which results are exported
         */
        void execute (std::shared_ptr <tex::Beamer> outDoc);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GETTERS          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * @returns: the timestamp of the first point of the traces
         */
        uint64_t getMinTimestamp () const;

        /**
         * Change the min timestamp
         * @info: insert zeros in the traces if this-> _minTimestamp > time, do nothing otherwise
         */
        void setMinTimestamp (uint64_t time);

        /**
         * @returns: the global usage of the machine
         */
        const std::vector <UsageTrace> & getGlobalUsage () const;

    private:

        /**
         * Load the cpu/memory usage of the machine
         */
        void loadUsage (const std::string & traceFile);

        /**
         * Load the energy consumption of the machine
         */
        void loadEnergy (const std::string & traceFile);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ==================================          USAGE EXPORT          ==================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Create the figure of cpu/ram usage
         */
        void createUsageFigure (std::shared_ptr <tex::Beamer> doc) ;

        /**
         * Create a label name
         */
        std::string createLabelName (const std::string & name) const;

        void createCPUFigures (std::shared_ptr <tex::Beamer> doc, std::map <std::string, std::shared_ptr<tex::Plot> > & cpu, std::vector <double> & cpuUsage);
        void createRAMFigures (std::shared_ptr <tex::Beamer> doc, std::map <std::string, std::shared_ptr<tex::Plot> > & cpu, std::vector <uint64_t> & ramUsage);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * =================================          ENERGY EXPORT          ==================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Create the figure of energy consumption
         */
        void createEnergyFigure (std::shared_ptr <tex::Beamer> doc) ;
    };


}
