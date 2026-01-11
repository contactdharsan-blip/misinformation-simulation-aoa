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

      // Assign to a school (if age appropriate)
      int schoolId = -1;
      if (age >= 5 && age <= 22) { // School age
        Location *school = towns[townId].getRandomSchool(rng);
        if (school && school->assignAgent(i)) {
          schoolId = school->id;
        }
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

private:
  // ========================================================================
  // DEMOGRAPHIC GENERATION HELPERS
  // ========================================================================

  // Generate age based on Phoenix 2020-2023 distribution
  int generateAge(double randomValue) {
    double cumulative = 0.0;

    // Approximated from census data: 16% Child, 10% Teen, 25% Young Adult, 32%
    // Adult, 17% Senior
    double child_w = 0.16;
    double teen_w = 0.10;
    double young_w = 0.25;
    double adult_w = 0.32;
    // senior_w implicitly 0.17

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

  // Generate ethnicity based on Phoenix 2020-2023 distribution
  EthnicGroup generateEthnicity(double randomValue) {
    // White 41.3%, Hispanic 41.8%, Black 7.4%, Asian 4.0%, Native 2.0%,
    // Multi 3.5%
    if (randomValue < 0.413)
      return EthnicGroup::WHITE;
    if (randomValue < 0.831)
      return EthnicGroup::HISPANIC;
    if (randomValue < 0.905)
      return EthnicGroup::BLACK;
    if (randomValue < 0.945)
      return EthnicGroup::ASIAN;
    if (randomValue < 0.965)
      return EthnicGroup::NATIVE_AMERICAN;
    return EthnicGroup::MULTIRACIAL;
  }

  // Generate Religious Denomination based on Phoenix demographics
  ReligiousDenomination generateDenomination(double randomValue) {
    // Catholic: 21%, Evangelical: 18%, Mainline: 9%, LDS: 5%, Jewish: 2%,
    // Buddhist: 3%, Hindu: 2%, Muslim: 1%, None: 39%
    if (randomValue < 0.21)
      return ReligiousDenomination::CATHOLIC;
    if (randomValue < 0.39)
      return ReligiousDenomination::EVANGELICAL;
    if (randomValue < 0.48)
      return ReligiousDenomination::MAINLINE;
    if (randomValue < 0.53)
      return ReligiousDenomination::LDS;
    if (randomValue < 0.55)
      return ReligiousDenomination::JEWISH;
    if (randomValue < 0.58)
      return ReligiousDenomination::BUDDHIST;
    if (randomValue < 0.60)
      return ReligiousDenomination::HINDU;
    if (randomValue < 0.61)
      return ReligiousDenomination::MUSLIM;
    return ReligiousDenomination::NONE;
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
