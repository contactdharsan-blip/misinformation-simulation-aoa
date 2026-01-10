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
                         cfg.school_capacity, cfg.religious_capacity);

      // Build flat location list
      for (auto &school : towns[i].schools) {
        allLocations.push_back(&school);
      }
      for (auto &religious : towns[i].religiousEstablishments) {
        allLocations.push_back(&religious);
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

      // Assign to a town (random distribution)
      int townId = townDist(rng);

      // Assign to a school (if age appropriate)
      int schoolId = -1;
      if (age >= 5 && age <= 22) { // School age
        Location *school = towns[townId].getRandomSchool(rng);
        if (school && school->assignAgent(i)) {
          schoolId = school->id;
        }
      }

      // Assign to a religious establishment (probabilistic)
      int religiousId = -1;
      std::uniform_real_distribution<double> religiousProb(0.0, 1.0);
      if (religiousProb(rng) < cfg.religious_participation_prob) {
        Location *religious = towns[townId].getRandomReligious(rng);
        if (religious && religious->assignAgent(i)) {
          religiousId = religious->id;
        }
      }

      agents.emplace_back(i, age, education, townId, schoolId, religiousId,
                          ethnicity);
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

private:
  // ========================================================================
  // DEMOGRAPHIC GENERATION HELPERS
  // ========================================================================

  // Generate age based on distribution
  int generateAge(double randomValue) {
    // Use hardcoded placeholder weights for now
    double cumulative = 0.0;

    double child_w = 0.15;
    double teen_w = 0.10;
    double young_w = 0.25;
    double adult_w = 0.30;

    cumulative += child_w;
    if (randomValue < cumulative) {
      std::uniform_int_distribution<int> dist(0, 12);
      return dist(rng);
    }

    cumulative += teen_w;
    if (randomValue < cumulative) {
      std::uniform_int_distribution<int> dist(13, 19);
      return dist(rng);
    }

    cumulative += young_w;
    if (randomValue < cumulative) {
      std::uniform_int_distribution<int> dist(20, 35);
      return dist(rng);
    }

    cumulative += adult_w;
    if (randomValue < cumulative) {
      std::uniform_int_distribution<int> dist(36, 60);
      return dist(rng);
    }

    // Senior
    std::uniform_int_distribution<int> dist(61, 90);
    return dist(rng);
  }

  // Generate ethnicity based on distribution
  EthnicGroup generateEthnicity(double randomValue) {
    // Equal distribution for now
    double perGroup = 1.0 / 4.0;

    if (randomValue < perGroup)
      return EthnicGroup::GROUP_A;
    if (randomValue < 2 * perGroup)
      return EthnicGroup::GROUP_B;
    if (randomValue < 3 * perGroup)
      return EthnicGroup::GROUP_C;
    return EthnicGroup::GROUP_D;
  }

  // Generate education level based on age
  int generateEducation(int age) {
    // Education correlates with age up to a point
    double mean = 2.5;
    double stddev = 1.2;

    if (age < 18) {
      mean = std::min(static_cast<double>(age) / 4.0, mean);
    }

    std::normal_distribution<double> dist(mean, stddev);
    int edu = static_cast<int>(std::round(dist(rng)));
    return std::max(0, std::min(5, edu)); // Clamp to 0-5
  }
};
