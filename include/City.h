#pragma once

#include "Agent.h"
#include "Configuration.h"
#include "Location.h"
#include "Town.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

// ============================================================================
// CITY CLASS
// Represents the simulation environment with towns, locations, and agents
// ============================================================================

class City {
public:
  // Towns in the city
  std::vector<Town> towns;

  // All locations (flat list for quick lookup)
  std::vector<Location *> allLocations;

  // Population
  std::vector<Agent> agents;

  // Random number generator
  std::mt19937 rng;

  // Constructor
  City(unsigned int seed = 42) : rng(seed) {}

  // ========================================================================
  // TOWN GENERATION
  // Creates towns with schools and religious establishments
  // ========================================================================
  void generateTowns() {
    auto &cfg = Configuration::instance();
    towns.clear();
    allLocations.clear();

    for (int i = 0; i < cfg.num_towns; ++i) {
      towns.emplace_back(i, cfg.schools_per_town, cfg.religious_per_town,
                         cfg.workplaces_per_town, cfg.school_capacity,
                         cfg.religious_capacity, cfg.workplace_capacity);

      // Build flat location list
      for (auto &school : towns[i].schools) {
        allLocations.push_back(&school);
      }
      for (auto &religious : towns[i].religiousEstablishments) {
        allLocations.push_back(&religious);
      }
      for (auto &work : towns[i].workplaces) {
        allLocations.push_back(&work);
      }
    }
  }

  // ========================================================================
  // POPULATION GENERATION
  // Creates agents and assigns them to towns and locations
  // ========================================================================
  void generatePopulation(int populationSize) {
    auto &cfg = Configuration::instance();
    agents.clear();
    agents.reserve(populationSize);

    std::uniform_real_distribution<double> uniformDist(0.0, 1.0);
    std::uniform_int_distribution<int> townDist(0, cfg.num_towns - 1);

    for (int i = 0; i < populationSize; ++i) {
      // Generate demographic properties
      int age = generateAge(uniformDist(rng));
      EthnicGroup ethnicity = generateEthnicity(uniformDist(rng));
      int education = generateEducation(age);
      ReligiousDenomination denomination =
          generateDenomination(uniformDist(rng));

      // Assign to a town (random distribution)
      int townId = townDist(rng);

      // Assign to a school (All people are assigned as requested)
      int schoolId = -1;
      Location *school = towns[townId].getRandomSchool(rng);
      if (school && school->assignAgent(i)) {
        schoolId = school->id;
      }

      // Assign to a religious establishment (probabilistic matching
      // denomination)
      int religiousId = -1;
      if (denomination != ReligiousDenomination::NONE) {
        Location *religious =
            towns[townId].getRandomReligiousOfDenomination(rng, denomination);
        if (religious && religious->assignAgent(i)) {
          religiousId = religious->id;
        }
      }

      // Assign to a workplace (based on education level)
      int workplaceId = -1;
      if (age >= 18 && education >= 0) {
        // Find a workplace in the town.
        // We could cluster by education: e.g. workplaces 0-2 for low edu, 3-5
        // for med, etc.
        if (!towns[townId].workplaces.empty()) {
          int numWork = towns[townId].workplaces.size();
          // Deterministic but distributed mapping from education to workplace
          // subset
          int workplaceIdx = (education * 2 + (i % 2)) % numWork;
          Location *work = &towns[townId].workplaces[workplaceIdx];
          if (work->assignAgent(i)) {
            workplaceId = work->id;
          }
        }
      }

      agents.emplace_back(i, age, education, townId, schoolId, religiousId,
                          workplaceId, ethnicity, denomination);
    }
  }

  // ========================================================================
  // NETWORK GENERATION
  // Creates connections based on shared locations
  // ========================================================================
  void generateNetwork() {
    auto &cfg = Configuration::instance();

    // Clear existing connections
    for (auto &agent : agents) {
      agent.connections.clear();
    }

    // For each agent, connect based on location overlap
    for (size_t i = 0; i < agents.size(); ++i) {
      Agent &agent = agents[i];
      std::uniform_real_distribution<double> probDist(0.0, 1.0);

      for (size_t j = i + 1; j < agents.size(); ++j) {
        Agent &other = agents[j];

        // Calculate connection probability based on shared locations
        double prob = agent.getInteractionProbability(other);

        // Create connection if probability check passes and haven't hit max
        if (probDist(rng) < prob) {
          if (static_cast<int>(agent.connections.size()) <
                  cfg.max_connections &&
              static_cast<int>(other.connections.size()) <
                  cfg.max_connections) {
            agent.connections.push_back(other.id);
            other.connections.push_back(agent.id);
          }
        }
      }
    }
  }

  // ========================================================================
  // GETTERS
  // ========================================================================

  // Get agents by town
  std::vector<int> getAgentsByTown(int townId) const {
    std::vector<int> result;
    for (const auto &agent : agents) {
      if (agent.homeTownId == townId) {
        result.push_back(agent.id);
      }
    }
    return result;
  }

  // Get agents by age group
  std::vector<int> getAgentsByAgeGroup(AgeGroup group) const {
    std::vector<int> result;
    for (const auto &agent : agents) {
      if (agent.getAgeGroup() == group) {
        result.push_back(agent.id);
      }
    }
    return result;
  }

  // Get agents by ethnicity
  std::vector<int> getAgentsByEthnicity(EthnicGroup group) const {
    std::vector<int> result;
    for (const auto &agent : agents) {
      if (agent.ethnicity == group) {
        result.push_back(agent.id);
      }
    }
    return result;
  }

  // Get agent by ID
  Agent &getAgent(int id) { return agents[id]; }

  const Agent &getAgent(int id) const { return agents[id]; }

  // Get population size
  size_t getPopulationSize() const { return agents.size(); }

  // Find a random new connection candidate for an agent
  // Returns -1 if no suitable candidate found
  int findRandomNewConnection(int agentId, int excludeId) {
    auto &cfg = Configuration::instance();
    Agent &agent = agents[agentId];

    // Check if agent has room for more connections
    if (static_cast<int>(agent.connections.size()) >= cfg.max_connections) {
      return -1;
    }

    // Build list of candidates (not self, not excludeId, not already connected)
    std::vector<int> candidates;
    for (size_t i = 0; i < agents.size(); ++i) {
      int candidateId = static_cast<int>(i);
      if (candidateId == agentId || candidateId == excludeId)
        continue;

      // Check if already connected
      bool alreadyConnected = false;
      for (int connId : agent.connections) {
        if (connId == candidateId) {
          alreadyConnected = true;
          break;
        }
      }
      if (alreadyConnected)
        continue;

      // Check if candidate has room
      if (static_cast<int>(agents[candidateId].connections.size()) >=
          cfg.max_connections)
        continue;

      candidates.push_back(candidateId);
    }

    if (candidates.empty())
      return -1;

    // Pick randomly
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
  }

private:
  // ========================================================================
  // DEMOGRAPHIC GENERATION HELPERS
  // ========================================================================

  // Generate age based on Phoenix 2020-2023 distribution
  int generateAge(double randomValue) {
    double cumulative = 0.0;

    // Use centralized constants
    cumulative += PHOENIX_AGE_PROBS[0]; // Child
    if (randomValue < cumulative) {
      std::uniform_int_distribution<int> dist(0, 12);
      return dist(rng);
    }

    cumulative += PHOENIX_AGE_PROBS[1]; // Teen
    if (randomValue < cumulative) {
      std::uniform_int_distribution<int> dist(13, 19);
      return dist(rng);
    }

    cumulative += PHOENIX_AGE_PROBS[2]; // Young Adult
    if (randomValue < cumulative) {
      std::uniform_int_distribution<int> dist(20, 35);
      return dist(rng);
    }

    cumulative += PHOENIX_AGE_PROBS[3]; // Adult
    if (randomValue < cumulative) {
      std::uniform_int_distribution<int> dist(36, 55);
      return dist(rng);
    }

    // Senior
    std::uniform_int_distribution<int> dist(56, 90);
    return dist(rng);
  }

  // Generate ethnicity based on Phoenix 2020-2023 distribution
  EthnicGroup generateEthnicity(double randomValue) {
    double cumulative = 0.0;

    cumulative += PHOENIX_ETHNICITY_PROBS[0];
    if (randomValue < cumulative)
      return EthnicGroup::WHITE;

    cumulative += PHOENIX_ETHNICITY_PROBS[1];
    if (randomValue < cumulative)
      return EthnicGroup::HISPANIC;

    cumulative += PHOENIX_ETHNICITY_PROBS[2];
    if (randomValue < cumulative)
      return EthnicGroup::BLACK;

    cumulative += PHOENIX_ETHNICITY_PROBS[3];
    if (randomValue < cumulative)
      return EthnicGroup::ASIAN;

    cumulative += PHOENIX_ETHNICITY_PROBS[4];
    if (randomValue < cumulative)
      return EthnicGroup::NATIVE_AMERICAN;

    return EthnicGroup::MULTIRACIAL;
  }

  // Generate Religious Denomination based on Phoenix demographics
  ReligiousDenomination generateDenomination(double randomValue) {
    double cumulative = PHOENIX_RELIGION_PROBS[0];

    // PHOENIX_RELIGION_PROBS[] order:
    // None(0), Catholic(1), Evan(2), Main(3), LDS(4), Other(5)
    // Map to Enum values:

    if (randomValue < cumulative)
      return ReligiousDenomination::NONE;

    cumulative += PHOENIX_RELIGION_PROBS[1];
    if (randomValue < cumulative)
      return ReligiousDenomination::CATHOLIC;

    cumulative += PHOENIX_RELIGION_PROBS[2];
    if (randomValue < cumulative)
      return ReligiousDenomination::EVANGELICAL;

    cumulative += PHOENIX_RELIGION_PROBS[3];
    if (randomValue < cumulative)
      return ReligiousDenomination::MAINLINE;

    cumulative += PHOENIX_RELIGION_PROBS[4];
    if (randomValue < cumulative)
      return ReligiousDenomination::LDS;

    // Remaining (Other)
    // Simple distribution for remaining small groups
    double rem = randomValue - cumulative;
    if (rem < 0.005)
      return ReligiousDenomination::JEWISH;
    if (rem < 0.009)
      return ReligiousDenomination::MUSLIM;
    if (rem < 0.013)
      return ReligiousDenomination::BUDDHIST;
    if (rem < 0.017)
      return ReligiousDenomination::HINDU;

    return ReligiousDenomination::NONE; // Fallback
  }

  // Generate education level based on age and Phoenix metrics
  int generateEducation(int age) {
    // 32.3% Bachelor's or higher for age 25+
    // Mapping 0-5 scale: 0=None, 1=Elem, 2=HS, 3=Associate, 4=Bachelor, 5=Grad

    double mean =
        2.8; // Shifted slightly higher to reflect Phoenix demographics
    double stddev = 1.0;

    if (age < 18) {
      mean = std::min(static_cast<double>(age) / 3.0, mean);
    } else if (age < 22) {
      mean = 2.0; // Most in HS range
    }

    std::normal_distribution<double> dist(mean, stddev);
    int edu = static_cast<int>(std::round(dist(rng)));
    return std::max(0, std::min(5, edu));
  }
};
