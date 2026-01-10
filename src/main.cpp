// ============================================================================
// SEDPNR Agent-Based Misinformation Simulation
// Main Entry Point
// ============================================================================

#include "Simulation.h"
#include <iostream>
#include <string>

// Note: Configuration is now managed by the Configuration singleton

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main(int argc, char *argv[]) {
  auto &cfg = Configuration::instance();

  // Load configuration from file if it exists
  cfg.load("parameters.cfg");

  std::cout << "=================================================="
            << std::endl;
  std::cout << "SEDPNR Agent-Based Misinformation Simulation" << std::endl;
  std::cout << "=================================================="
            << std::endl;

  // Parse command line arguments (optional overrides)
  if (argc >= 2) {
    cfg.population = std::stoi(argv[1]);
  }
  if (argc >= 3) {
    cfg.timesteps = std::stoi(argv[2]);
  }

  std::cout << "\nConfiguration:" << std::endl;
  std::cout << "  Towns:       " << cfg.num_towns << std::endl;
  std::cout << "  Population:  " << cfg.population << std::endl;
  std::cout << "  Time steps:  " << cfg.timesteps << std::endl;
  std::cout << "  Random seed: " << cfg.seed << std::endl;

  // Create simulation
  Simulation sim(cfg.seed);

  std::cout << "\nInitializing city and population..." << std::endl;
  sim.initialize(cfg.population);

  std::cout << "City generated with " << sim.city.getPopulationSize()
            << " agents" << std::endl;

  // Add claims
  std::cout << "\nAdding claims..." << std::endl;

  // Add a truth claim
  Claim truth = Claim::createTruth(0, "Factual_Claim");
  sim.addClaim(truth, 5); // Default 5 propagators
  std::cout << "  Added: " << truth.name
            << " (Truth) with 5 initial propagators" << std::endl;

  // Add misinformation claims
  Claim misinfo1 = Claim::createMisinformation(1, "Misinfo_Claim_1");
  sim.addClaim(misinfo1, 5);
  std::cout << "  Added: " << misinfo1.name
            << " (Misinformation) with 5 initial propagators" << std::endl;

  Claim misinfo2 = Claim::createMisinformation(2, "Misinfo_Claim_2");
  sim.addClaim(misinfo2, 5);
  std::cout << "  Added: " << misinfo2.name
            << " (Misinformation) with 5 initial propagators" << std::endl;

  // Run simulation
  std::cout << "\nRunning simulation..." << std::endl;
  sim.run(cfg.timesteps);

  // Output results
  std::cout << "\nWriting results..." << std::endl;
  sim.outputResults("output/simulation_results.csv");

  // Print summary
  sim.outputSummary();

  std::cout << "\n=================================================="
            << std::endl;
  std::cout << "Simulation complete!" << std::endl;
  std::cout << "=================================================="
            << std::endl;

  return 0;
}
