#include "machine.hh"
#include <fstream>
#include <cmath>
#include "utils.hh"

using namespace rd_utils;
using namespace rd_utils::utils;

namespace analyser {

    Machine::Machine () {}

    void Machine::configure (const std::string & traceDir, const config::ConfigNode & cfg) {
        this-> _minTimestamp = -1;
        try {
            this-> _name = cfg ["hostname"].getStr ();
            this-> loadUsage (utils::join_path (traceDir, this-> _name + "/cgroups.csv"));
            this-> loadEnergy (utils::join_path (traceDir, this-> _name + "/energy.csv"));
        } catch (const std::runtime_error & err) {
            this-> _minTimestamp = 0;
            throw std::runtime_error (std::string ("Failed to load machine traces (") + err.what () + ")");
        }
    }

    void Machine::loadUsage (const std::string & filename) {
        std::ifstream csv (filename);
        std::string line, head;
        std::ignore = std::getline (csv, head);

        while (std::getline (csv, line)) {
            auto splits = utils::splitByString (line, ";");
            if (splits.size () != 5) throw std::runtime_error ("Cgroup usage file malformed");

            uint64_t timestamp = std::ceil (std::stod (splits [0]));
            auto name = splits [1];
            if (name == "#SYSTEM") {
                if (timestamp < this-> _minTimestamp) this-> _minTimestamp = timestamp;
                this-> _usage.push_back ({});
                this-> _globalUsage.push_back (UsageTrace {.cpu = 0, .mem_anon = MemorySize::B (0), .mem_file = MemorySize::B (0)});

                continue;
            } 

            auto cpu = (double) (std::stoul (splits [2], nullptr, 0)) / 1e7; // 10_000_000; to have a percentage
            auto mem_anon = std::stoul (splits [3], nullptr, 0);
            auto mem_file = 0; //std::stoul (splits [4], nullptr, 0);

            this-> _groups.emplace (name);
            this-> _usage.back () [name] = UsageTrace {.cpu = cpu, .mem_anon = MemorySize::B (mem_anon), .mem_file = MemorySize::B (mem_file)};
            this-> _globalUsage.back ().cpu += cpu;
            this-> _globalUsage.back ().mem_anon += MemorySize::B (mem_anon);
            this-> _globalUsage.back ().mem_file += MemorySize::B (mem_file);
        }
    }

    void Machine::loadEnergy (const std::string & filename) {
        std::ifstream csv (filename);
        std::string line, head;
        std::ignore = std::getline (csv, head);

        auto last = EnergyTrace {.pdu = 0, .cpu = 0, .ram = 0};

        while (std::getline (csv, line)) {
            auto splits = utils::splitByString (line, ";");
            if (splits.size () != 5) throw std::runtime_error ("Cgroup usage file malformed");

            auto timestamp = std::stod (splits [0]);
            if (this-> _minTimestamp > timestamp) this-> _minTimestamp = timestamp;

            auto pdu = std::stof (splits [1]);
            auto cpu = std::stof (splits [2]);
            auto ram = std::stof (splits [3]);

            this-> _energy.push_back (EnergyTrace {.pdu = pdu - last.pdu, .cpu = cpu - last.cpu, .ram = ram - last.ram});
            last = EnergyTrace {.pdu = pdu, .cpu = cpu, .ram = ram};
        }
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GET/SET          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    uint64_t Machine::getMinTimestamp () const {
        return this-> _minTimestamp;
    }

    void Machine::setMinTimestamp (uint64_t timestamp) {
        if (this-> _minTimestamp > timestamp) {
            for (uint64_t i = 0 ; i < this-> _minTimestamp - timestamp ; i++) {
                this-> _globalUsage.insert (this-> _globalUsage.begin (), UsageTrace {.cpu = 0, .mem_anon = MemorySize::B (0), .mem_file = MemorySize::B (0)});
                this-> _usage.insert (this-> _usage.begin (), {});
            }
        }
    }

    const std::vector <UsageTrace> & Machine::getGlobalUsage () const {
        return this-> _globalUsage;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          EXPORT          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Machine::execute (std::shared_ptr <tex::Beamer> doc) {
        this-> createUsageFigure (doc);
        this-> createEnergyFigure (doc);
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          ENERGY EXPORT          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Machine::createEnergyFigure (std::shared_ptr <tex::Beamer> doc) {
        auto pduPlot = std::make_shared <tex::Plot> ();
        auto cpuPlot = std::make_shared <tex::Plot> ();
        auto ramPlot = std::make_shared <tex::Plot> ();

        pduPlot-> legend ("PDU");
        cpuPlot-> legend ("CPU");
        ramPlot-> legend ("RAM");

        std::vector <double> pdu;
        std::vector <double> cpu;
        std::vector <double> ram;

        for (auto & it : this-> _energy) {
            pduPlot-> append (it.pdu);
            cpuPlot-> append (it.cpu);
            ramPlot-> append (it.ram);

            pdu.push_back (it.pdu);
            cpu.push_back (it.cpu);
            ram.push_back (it.ram);
        }

        auto energyFigure = tex::AxisFigure ("energy_" + this-> _name)
            .caption ("Energy consumption of " + this-> _name)
            .ylabel ("Watts")
            .xlabel ("seconds")
            .addPlot (pduPlot)
            .addPlot (cpuPlot)
            .addPlot (ramPlot)
            ;

        auto table = tex::TableFigure ("energy_table_" + this-> _name, {"", "PDU", "CPU", "RAM"});
        table.addRow ({"min",
                std::to_string ((uint64_t) analyser::min (pdu)),
                std::to_string ((uint64_t) analyser::min (cpu)),
                std::to_string ((uint64_t) analyser::min (ram))});

        table.addRow ({"max",
                std::to_string ((uint64_t) analyser::max (pdu)),
                std::to_string ((uint64_t) analyser::max (cpu)),
                std::to_string ((uint64_t) analyser::max (ram))});

        double meanPDU = analyser::mean (pdu);
        double meanCPU = analyser::mean (cpu);
        double meanRAM = analyser::mean (ram);

        table.addRow ({"mean",
                std::to_string ((uint64_t) meanPDU),
                std::to_string ((uint64_t) meanCPU),
                std::to_string ((uint64_t) meanRAM)});

        table.addRow ({"std",
                std::to_string ((uint64_t) ::sqrt (analyser::variance (meanPDU, pdu))),
                std::to_string ((uint64_t) ::sqrt (analyser::variance (meanCPU, cpu))),
                std::to_string ((uint64_t) ::sqrt (analyser::variance (meanRAM, ram)))});

        table.addRow ({"sum (J)",
                std::to_string ((uint64_t) analyser::sum (pdu)),
                std::to_string ((uint64_t) analyser::sum (cpu)),
                std::to_string ((uint64_t) analyser::sum (ram))});

        table.resize (0.75);

        auto minipage = tex::MiniPageFigure ("minipage_energy_" + this-> _name)
            .addFigure (std::make_shared<tex::AxisFigure> (energyFigure), 75)
            .addFigure (std::make_shared <tex::TableFigure> (table), 19);

        doc-> addFigure ("cluster", std::make_shared <tex::MiniPageFigure> (minipage));
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ==================================          USAGE EXPORT          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    void Machine::createUsageFigure (std::shared_ptr <tex::Beamer> doc) {
        std::map <std::string, std::shared_ptr <tex::Plot> > cpu;
        std::map <std::string, std::shared_ptr <tex::Plot> > ram;

        cpu.emplace ("__GLOBAL__", std::make_shared <tex::Plot> ());
        cpu ["__GLOBAL__"]-> legend ("Sum");

        ram.emplace ("__GLOBAL__", std::make_shared <tex::Plot> ());
        ram ["__GLOBAL__"]-> legend ("Sum");
        std::vector <uint64_t> rams;
        std::vector <double> cpus;

        for (auto & sum : this-> _globalUsage) {
            cpu ["__GLOBAL__"]-> append (sum.cpu);
            ram ["__GLOBAL__"]-> append ((sum.mem_anon + sum.mem_file).megabytes ());
            cpus.push_back (sum.cpu);

            auto kb = (sum.mem_anon + sum.mem_file).megabytes ();
            rams.push_back (kb);
        }

        for (auto & name : this-> _groups) {
            auto uniqName = "__LOCAL__" + name;
            auto labelName = this-> createLabelName (name);

            cpu.emplace (uniqName, std::make_shared <tex::Plot> ());
            cpu [uniqName]-> legend (labelName);

            ram.emplace (uniqName, std::make_shared <tex::Plot> ());
            ram [uniqName]-> legend (labelName);

            for (auto & usage : this-> _usage) {
                if (usage.find (name) != usage.end ()) {
                    cpu [uniqName]-> append (usage [name].cpu);
                    ram [uniqName]-> append ((usage [name].mem_anon + usage [name].mem_file).megabytes ());
                } else {
                    cpu [uniqName]-> append (0);
                    ram [uniqName]-> append (0);
                }
            }
        }

        this-> createCPUFigures (doc, cpu, cpus);
        this-> createRAMFigures (doc, ram, rams);
    }

    void Machine::createCPUFigures (std::shared_ptr <tex::Beamer> doc, std::map <std::string, std::shared_ptr<tex::Plot> > & cpu, std::vector <double> & cpuUsage) {
        std::sort (cpuUsage.begin (), cpuUsage.end ());

        auto table = tex::TableFigure ("cpu_table_" + this-> _name, {"", "\\%"});
        table.addRow ({"min", std::to_string ((uint64_t) analyser::min (cpuUsage))});
        table.addRow ({"max", std::to_string ((uint64_t) analyser::max (cpuUsage))});

        double mean = analyser::mean (cpuUsage);
        table.addRow ({"mean", std::to_string ((uint64_t) mean)});
        table.addRow ({"std", std::to_string ((uint64_t) ::sqrt (analyser::variance (mean, cpuUsage)))});
        table.addRow ({"sum", std::to_string ((uint64_t) analyser::sum (cpuUsage))});

        auto cpuFigure = tex::AxisFigure ("cpu_" + this-> _name)
            .caption ("CPU Usage of " + this-> _name)
            .ylabel ("\\% CPU")
            .xlabel ("seconds")
            ;

        for (auto & c : cpu) {
            cpuFigure.addPlot (c.second);
        }

        auto minipage = tex::MiniPageFigure ("minipage_cpu_" + this-> _name)
            .addFigure (std::make_shared<tex::AxisFigure> (cpuFigure), 75)
            .addFigure (std::make_shared <tex::TableFigure> (table), 19);

        doc-> addFigure ("cluster", std::make_shared <tex::MiniPageFigure> (minipage));
    }

    void Machine::createRAMFigures (std::shared_ptr <tex::Beamer> doc, std::map <std::string, std::shared_ptr<tex::Plot> > & ram, std::vector <uint64_t> & ramUsage) {
        std::sort (ramUsage.begin (), ramUsage.end ());
        auto mi = analyser::min (ramUsage), mx = analyser::max (ramUsage);

        auto table = tex::TableFigure ("ram_table_" + this-> _name, {"", "MB"});
        table.addRow ({"min", std::to_string (mi)});
        table.addRow ({"max", std::to_string (mx)});
        table.addRow ({"dist", std::to_string (mx - mi)});
        table.addRow ({"sum", std::to_string ((uint64_t) analyser::sum (ramUsage))});

        auto ramFigure = tex::AxisFigure ("ram_" + this-> _name)
            .caption ("RAM Usage of " + this-> _name)
            .ylabel ("Memory size in MB")
            .xlabel ("seconds")
            ;

        for (auto & r : ram) {
            ramFigure.addPlot (r.second);
        }

        auto minipage = tex::MiniPageFigure ("minipage_ram_" + this-> _name)
            .addFigure (std::make_shared<tex::AxisFigure> (ramFigure), 75)
            .addFigure (std::make_shared <tex::TableFigure> (table), 19);

        doc-> addFigure ("cluster", std::make_shared <tex::MiniPageFigure> (minipage));
    }

    std::string Machine::createLabelName (const std::string & name) const {
        std::string labelName = "";
        if (name.find ("/") != std::string::npos) {
            labelName = utils::findAndReplaceAll (utils::get_filename (name), {{"#", "\\_"}, {"/", "\\_"}});
        } else {
            labelName = utils::findAndReplaceAll (name, {{"#", "\\_"}, {"/", "\\_"}});
        }

        if (labelName.size () > 10) { labelName = labelName.substr (0, 5) + "[...]" + labelName.substr (labelName.size () - 5); }
        return labelName;
    }

}
