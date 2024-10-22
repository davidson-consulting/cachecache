
#include <iostream>
#include <rd_utils/concurrency/timer.hh>
#include <rd_utils/utils/print.hh>
#include <rd_utils/concurrency/taskpool.hh>
#include <rd_utils/foreign/CLI11.hh>

#include <simulation.hh>

using namespace cachecache_sim;

void run (uint32_t index, std::string configPath, std::string outPath) {
    std::string currPath = outPath;
    if (currPath == "") { currPath = ".out/" + std::to_string (index) + "/"; }
    else { currPath += "/" + std::to_string (index) + "/"; }

    rd_utils::utils::create_directory (currPath, true);

    Simulation sim;
    sim.configure (configPath, currPath);
    sim.execute ();
}


auto main(int argc, char *argv[]) -> int {
  std::string configPath, outPath;
  int nbRun = 1;
  CLI::App app;
  app.add_option ("-c,--config-path", configPath, "the path of the configuration file");
  app.add_option ("-o,--output-path", outPath, "the output path");
  app.add_option ("-n,--nb-run", nbRun, "the number of executions");

  try {
    app.parse (argc, argv);
  } catch (const CLI::ParseError &e) {
    exit (app.exit (e));
  }

  rd_utils::concurrency::TaskPool pool (std::min (8, nbRun));

  if (configPath == "") { configPath = "config.toml";  }
  if (outPath == "") { outPath = ".out/"; }

  for (uint32_t i = 0 ; i < nbRun ; i ++) {
    pool.submit (&run, i, configPath, outPath);
  }

  pool.join ();
}
