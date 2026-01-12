#pragma once

#include "City.h"
#include "Claim.h"
#include "Configuration.h"
#include "SEDPNR.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

// Note: Simulation parameters are now managed by Configuration::instance()

// ============================================================================
// STATE COUNTS STRUCTURE
// ============================================================================

struct StateCounts {
  int susceptible = 0;
  int exposed = 0;
  int doubtful = 0;
  int propagating = 0;
  int notSpreading = 0;
  int recovered = 0;

  int total() const {
    return susceptible + exposed + doubtful + propagating + notSpreading +
           recovered;
  }
};

// ============================================================================
// SIMULATION CLASS
// Main simulation engine for SEDPNR model
// ============================================================================

class Simulation {
public:
  // City containing agents
  City city;

  // Claims being simulated
  std::vector<Claim> claims;

  // Current simulation time
  int currentTime;

  // State counts over time for each claim
  // claim_id -> time -> counts
  std::map<int, std::vector<StateCounts>> stateHistory;

  // Random number generator
  std::mt19937 rng;
  std::ofstream spatialFile;

  // Constructor
  Simulation(unsigned int seed = 42) : currentTime(0), rng(seed) {
    spatialFile.open("output/spatial_data.csv");
    if (spatialFile.is_open()) {
      spatialFile
          << "Time,AgentId,TownId,SchoolId,ReligiousId,WorkplaceId,ClaimId,"
             "State,IsMisinformation,Ethnicity,Denomination\n";
    }
  }

  ~Simulation() {
    if (spatialFile.is_open()) {
      spatialFile.close();
    }
  }

  // ========================================================================
  // INITIALIZATION
  // ========================================================================

  void initialize(int population) {
    city = City(rng());
    city.generateTowns();
    city.generatePopulation(population);
    city.generateNetwork();
    currentTime = 0;
    stateHistory.clear();
  }

  // Add a claim to the simulation
  void addClaim(const Claim &claim, int initialPropagators = 10) {
    Claim c = claim;
    c.originTime = currentTime;
    claims.push_back(c);

    // Initialize state history for this claim
    stateHistory[c.claimId] = std::vector<StateCounts>();

    // Set initial propagators
    if (initialPropagators > 0 && city.getPopulationSize() > 0) {
      std::uniform_int_distribution<size_t> dist(0,
                                                 city.getPopulationSize() - 1);

      for (int i = 0; i < initialPropagators &&
                      i < static_cast<int>(city.getPopulationSize());
           ++i) {
        size_t agentIdx = dist(rng);
        city.agents[agentIdx].setState(c.claimId, SEDPNRState::PROPAGATING);

        if (c.originAgentId < 0) {
          claims.back().originAgentId = static_cast<int>(agentIdx);
        }
      }
    }
  }

  // Add a claim with specific number of propagators per town
  void addClaimPerDistrict(const Claim &claim, int propagatorsPerTown = 5) {
    Claim c = claim;
    c.originTime = currentTime;
    claims.push_back(c);

    // Initialize state history for this claim
    stateHistory[c.claimId] = std::vector<StateCounts>();

    // For each town, find agents and pick some
    std::map<int, std::vector<size_t>> townToAgents;
    for (size_t i = 0; i < city.agents.size(); ++i) {
      townToAgents[city.agents[i].homeTownId].push_back(i);
    }

    for (auto it = townToAgents.begin(); it != townToAgents.end(); ++it) {
      std::vector<size_t> indices = it->second;
      std::shuffle(indices.begin(), indices.end(), rng);

      for (int i = 0;
           i < propagatorsPerTown && i < static_cast<int>(indices.size());
           ++i) {
        size_t agentIdx = indices[i];
        city.agents[agentIdx].setState(c.claimId, SEDPNRState::PROPAGATING);

        if (claims.back().originAgentId < 0) {
          claims.back().originAgentId = static_cast<int>(agentIdx);
        }
      }
    }
  }

  // ========================================================================
  // SIMULATION STEP
  // ========================================================================

  void step() {
    std::uniform_real_distribution<double> uniformDist(0.0, 1.0);

    // Process each claim
    for (const auto &claim : claims) {
      // Create a copy of current states to avoid order-dependent updates
      std::map<int, SEDPNRState> newStates;

      for (auto &agent : city.agents) {
        SEDPNRState currentState = agent.getState(claim.claimId);
        SEDPNRState newState = currentState;

        switch (currentState) {
        case SEDPNRState::SUSCEPTIBLE:
          newState = processSusceptible(agent, claim, uniformDist);
          break;

        case SEDPNRState::EXPOSED:
          newState = processExposed(agent, claim, uniformDist);
          break;

        case SEDPNRState::DOUBTFUL:
          newState = processDoubtful(agent, claim, uniformDist);
          break;

        case SEDPNRState::PROPAGATING:
          newState = processPropagating(agent, claim, uniformDist);
          break;

        case SEDPNRState::NOT_SPREADING:
          newState = processNotSpreading(agent, claim, uniformDist);
          break;

        case SEDPNRState::RECOVERED:
          // Recovered agents stay recovered
          break;

        default:
          break;
        }

        newStates[agent.id] = newState;
        agent.incrementTimeInState(claim.claimId);
      }

      // Apply new states
      for (auto &agent : city.agents) {
        agent.setState(claim.claimId, newStates[agent.id]);
      }
    }

    // Record state counts
    if (currentTime % Configuration::instance().output_interval == 0) {
      recordStateCounts();
      recordSpatialSnapshot();
    }

    currentTime++;
  }

  void recordSpatialSnapshot() {
    if (!spatialFile.is_open())
      return;

    for (const auto &claim : claims) {
      for (const auto &agent : city.agents) {
        SEDPNRState state = agent.getState(claim.claimId);
        // Record if not susceptible OR if configured to record full snapshot
        if (Configuration::instance().full_spatial_snapshot ||
            state != SEDPNRState::SUSCEPTIBLE || currentTime == 0) {
          spatialFile << currentTime << "," << agent.id << ","
                      << agent.homeTownId << "," << agent.schoolLocationId
                      << "," << agent.religiousLocationId << ","
                      << agent.workplaceLocationId << "," << claim.claimId
                      << "," << static_cast<int>(state) << ","
                      << (claim.isMisinformation ? 1 : 0) << ","
                      << static_cast<int>(agent.ethnicity) << ","
                      << static_cast<int>(agent.denomination) << "\n";
        }
      }
    }
  }

  // ========================================================================
  // RUN SIMULATION
  // ========================================================================

  void run(int timeSteps = 500) {
    for (int t = 0; t < timeSteps; ++t) {
      step();

      // Progress indicator
      if (t % 100 == 0) {
        std::cout << "Time step: " << t << "/" << timeSteps << std::endl;
      }
    }
  }

  // ========================================================================
  // OUTPUT RESULTS
  // ========================================================================

  void
  outputResults(const std::string &filename = "output/simulation_results.csv") {
    std::ofstream file(filename);

    if (!file.is_open()) {
      std::cerr << "Error: Could not open output file: " << filename
                << std::endl;
      return;
    }

    // Write header
    file << "Time,ClaimId,ClaimName,IsMisinformation,"
         << "Susceptible,Exposed,Doubtful,Propagating,NotSpreading,Recovered\n";

    // Write data for each claim
    for (const auto &claim : claims) {
      const auto &history = stateHistory[claim.claimId];

      for (size_t t = 0; t < history.size(); ++t) {
        const auto &counts = history[t];
        file << t << "," << claim.claimId << "," << claim.name << ","
             << (claim.isMisinformation ? "true" : "false") << ","
             << counts.susceptible << "," << counts.exposed << ","
             << counts.doubtful << "," << counts.propagating << ","
             << counts.notSpreading << "," << counts.recovered << "\n";
      }
    }

    file.close();
    std::cout << "Results written to: " << filename << std::endl;
  }

  // Output summary statistics
  void outputSummary() {
    std::cout << "\n=== Simulation Summary ===" << std::endl;
    std::cout << "Population: " << city.getPopulationSize() << std::endl;
    std::cout << "Time steps: " << currentTime << std::endl;
    std::cout << "Claims: " << claims.size() << std::endl;

    for (const auto &claim : claims) {
      std::cout << "\n--- " << claim.name << " (" << claim.getTypeString()
                << ") ---" << std::endl;

      if (!stateHistory[claim.claimId].empty()) {
        const auto &finalCounts = stateHistory[claim.claimId].back();
        std::cout << "Final state distribution:" << std::endl;
        std::cout << "  Susceptible:   " << finalCounts.susceptible
                  << std::endl;
        std::cout << "  Exposed:       " << finalCounts.exposed << std::endl;
        std::cout << "  Doubtful:      " << finalCounts.doubtful << std::endl;
        std::cout << "  Propagating:   " << finalCounts.propagating
                  << std::endl;
        std::cout << "  Not-Spreading: " << finalCounts.notSpreading
                  << std::endl;
        std::cout << "  Recovered:     " << finalCounts.recovered << std::endl;
      }
    }
  }

  // Get latest state counts for a claim
  StateCounts getLatestStateCounts(int claimId) const {
    auto it = stateHistory.find(claimId);
    if (it != stateHistory.end() && !it->second.empty()) {
      return it->second.back();
    }
    return StateCounts();
  }

private:
  // ========================================================================
  // STATE TRANSITION PROCESSORS
  // ========================================================================

  // Process susceptible agent (S -> E)
  SEDPNRState processSusceptible(Agent &agent, const Claim &claim,
                                 std::uniform_real_distribution<double> &dist) {
    auto &cfg = Configuration::instance();

    // Calculate effective exposure from propagators, weighted by similarity
    double effectiveExposure = 0.0;
    for (int connId : agent.connections) {
      const Agent &other = city.getAgent(connId);
      if (other.getState(claim.claimId) == SEDPNRState::PROPAGATING) {
        // Multiplier based on similarity (homophily)
        // Strong Homophily = High Confirmation Bias (Identity-based trust)
        // We use power function to disproportionately weight similar agents
        double similarity = agent.calculateSimilarity(other);
        effectiveExposure += std::pow(similarity, cfg.homophily_strength);
      }
    }

    if (effectiveExposure > 0.0) {
      // Calculate exposure probability
      double baseProb = cfg.prob_s_to_e;

      // If misinformation, it spreads FASTER (more effective
      // exposure/frequency), not necessarily because people are more gullible
      // (higher base prob). So we apply the multiplier to the EXPOSURE count.
      if (claim.isMisinformation) {
        effectiveExposure *= cfg.misinfo_multiplier;
      }

      // Modify by interactions (weighted by similarity)
      // We use effectiveExposure as the exponent, treating "1.0 similarity" as
      // one standard contact
      double prob = 1.0 - std::pow(1.0 - baseProb, effectiveExposure);

      // Modify by claim passing frequency
      prob *= agent.getClaimPassingFrequency();

      if (dist(rng) < prob) {
        return SEDPNRState::EXPOSED;
      }
    }

    return SEDPNRState::SUSCEPTIBLE;
  }

  // Process exposed agent (E -> D)
  SEDPNRState processExposed(Agent &agent, const Claim &claim,
                             std::uniform_real_distribution<double> &dist) {
    auto &cfg = Configuration::instance();

    // REQUIREMENT: Must have connections to someone who has adopted (P or N)
    // to progress to Doubtful (social reinforcement)
    bool hasReinforcement = false;
    for (int connId : agent.connections) {
      SEDPNRState s = city.getAgent(connId).getState(claim.claimId);
      if (s == SEDPNRState::PROPAGATING || s == SEDPNRState::NOT_SPREADING) {
        hasReinforcement = true;
        break;
      }
    }

    if (hasReinforcement && agent.getTimeInState(claim.claimId) >= 0) {
      if (dist(rng) < cfg.prob_e_to_d) {
        return SEDPNRState::DOUBTFUL;
      }
    }

    return SEDPNRState::EXPOSED;
  }

  // Process doubtful agent (D -> P, N, or R)
  SEDPNRState processDoubtful(Agent &agent, const Claim &claim,
                              std::uniform_real_distribution<double> &dist) {
    auto &cfg = Configuration::instance();

    if (agent.getTimeInState(claim.claimId) >= 0) {
      // Calculate adoption threshold based on claim type and agent credibility
      double threshold =
          claim.isMisinformation ? cfg.misinfo_threshold : cfg.truth_threshold;

      // Credibility affects rejection probability
      double rejectionBonus =
          agent.credibilityValue * cfg.credibility_rejection_weight;

      double roll = dist(rng);

      // Adjusted probabilities
      double probReject = cfg.prob_d_to_r + rejectionBonus;
      double probPropagate = cfg.prob_d_to_p * (1.0 - threshold);
      double probNotSpread = cfg.prob_d_to_n;

      // REQUIREMENT: Validating social proof for adoption (P or N)
      // If no neighbors are P or N, you can't adopt, but you CAN reject.
      bool hasReinforcement = false;
      for (int connId : agent.connections) {
        SEDPNRState s = city.getAgent(connId).getState(claim.claimId);
        if (s == SEDPNRState::PROPAGATING || s == SEDPNRState::NOT_SPREADING) {
          hasReinforcement = true;
          break;
        }
      }

      // Truth claims should not be rejected/recovered from
      double actualProbReject = claim.isMisinformation ? probReject : 0.0;

      if (roll < actualProbReject) {
        return SEDPNRState::RECOVERED;
      } else if (hasReinforcement) {
        // Only allow adoption if reinforced
        if (roll < actualProbReject + probPropagate) {
          return SEDPNRState::PROPAGATING;
        } else if (roll < actualProbReject + probPropagate + probNotSpread) {
          return SEDPNRState::NOT_SPREADING;
        }
      }
    }

    return SEDPNRState::DOUBTFUL;
  }

  // Process propagating agent (P -> N or R)
  SEDPNRState processPropagating(Agent &agent, const Claim &claim,
                                 std::uniform_real_distribution<double> &dist) {
    auto &cfg = Configuration::instance();
    if (agent.getTimeInState(claim.claimId) >= 0) {
      double roll = dist(rng);

      // Truth claims should not be recovered from
      double probPtoR = claim.isMisinformation ? cfg.prob_p_to_r : 0.0;

      if (roll < probPtoR) {
        return SEDPNRState::RECOVERED;
      } else if (roll < probPtoR + cfg.prob_p_to_n) {
        return SEDPNRState::NOT_SPREADING;
      }
    }

    return SEDPNRState::PROPAGATING;
  }

  // Process not-spreading agent (N -> R)
  SEDPNRState
  processNotSpreading(Agent & /*agent*/, const Claim &claim,
                      std::uniform_real_distribution<double> &dist) {
    auto &cfg = Configuration::instance();

    // Truth claims should not be recovered from
    double probNtoR = claim.isMisinformation ? cfg.prob_n_to_r : 0.0;

    if (dist(rng) < probNtoR) {
      return SEDPNRState::RECOVERED;
    }

    return SEDPNRState::NOT_SPREADING;
  }

  // ========================================================================
  // STATE COUNTING
  // ========================================================================

  void recordStateCounts() {
    for (const auto &claim : claims) {
      StateCounts counts;

      for (const auto &agent : city.agents) {
        switch (agent.getState(claim.claimId)) {
        case SEDPNRState::SUSCEPTIBLE:
          counts.susceptible++;
          break;
        case SEDPNRState::EXPOSED:
          counts.exposed++;
          break;
        case SEDPNRState::DOUBTFUL:
          counts.doubtful++;
          break;
        case SEDPNRState::PROPAGATING:
          counts.propagating++;
          break;
        case SEDPNRState::NOT_SPREADING:
          counts.notSpreading++;
          break;
        case SEDPNRState::RECOVERED:
          counts.recovered++;
          break;
        default:
          break;
        }
      }

      stateHistory[claim.claimId].push_back(counts);
    }
  }

  // ========================================================================
  // SPATIAL DATA OUTPUT
  // Outputs agent positions and states for map visualization
  // ========================================================================
  void outputSpatialData(const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open spatial data file " << filename
                << std::endl;
      return;
    }

    file << "AgentId,X,Y,Time,ClaimId,State,IsMisinformation\n";

    // We'll iterate through all claims and then through time/history
    // But wait, the stateHistory only stores counts.
    // We need to capture agent-level snapshots during the simulation.
  }
};
