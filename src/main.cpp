// ============================================================================
// SEDPNR Agent-Based Misinformation Simulation
// Main Entry Point
// ============================================================================

#include "../include/Simulation.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

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
  sim.addClaim(truth, 10); // 10 initial propagators
  std::cout << "  Added: " << truth.name
            << " (Truth) with 10 initial propagators" << std::endl;

  // Add misinformation claims (5 propagators per district)
  Claim misinfo1 = Claim::createMisinformation(1, "Misinfo_Claim_1");
  sim.addClaimPerDistrict(misinfo1, 5);
  std::cout << "  Added: " << misinfo1.name
            << " (Misinformation) with 5 propagators per district" << std::endl;

  // Run simulation
  std::cout << "\nRunning controllable simulation..." << std::endl;
  std::cout
      << "Controls: [Enter] to step, [R] to run continuously, [P] to pause"
      << std::endl;

  bool continuous = true; // Auto-run to completion
  for (int t = 0; t < cfg.timesteps; ++t) {
    sim.step();

    // Print header every 20 steps or at start
    if (t % 20 == 0) {
      std::cout << "\n"
                << std::setw(6) << "Step" << " | " << std::setw(15) << "Claim"
                << " | " << std::setw(4) << "S" << " | " << std::setw(4) << "E"
                << " | " << std::setw(4) << "D" << " | " << std::setw(4) << "P"
                << " | " << std::setw(4) << "N" << " | " << std::setw(4) << "R"
                << std::endl;
      std::cout << std::string(65, '-') << std::endl;
    }

    // Print stats for each claim
    for (const auto &claim : sim.claims) {
      StateCounts sc = sim.getLatestStateCounts(claim.claimId);
      std::cout << std::setw(6) << t << " | " << std::setw(15)
                << claim.name.substr(0, 15) << " | " << std::setw(4)
                << sc.susceptible << " | " << std::setw(4) << sc.exposed
                << " | " << std::setw(4) << sc.doubtful << " | " << std::setw(4)
                << sc.propagating << " | " << std::setw(4) << sc.notSpreading
                << " | " << std::setw(4) << sc.recovered << std::endl;
    }

    if (!continuous) {
      std::string input;
      std::getline(std::cin, input);
      if (input == "r" || input == "R") {
        continuous = true;
      }
    } else {
      // Check for pause if possible (non-blocking char read is complex in
      // standard C++, we'll just run to end if 'R' is pressed, or use a small
      // sleep)
      // For now, once 'R' is pressed, it runs to completion but shows stats.
    }
  }

  // Output results
  std::cout << "\nWriting results..." << std::endl;
  sim.outputResults("output/simulation_results.csv");

  // Print final summary
  sim.outputSummary();

  std::cout << "\n=================================================="
            << std::endl;
  std::cout << "Simulation complete!" << std::endl;
  std::cout << "=================================================="
            << std::endl;

  return 0;
}
