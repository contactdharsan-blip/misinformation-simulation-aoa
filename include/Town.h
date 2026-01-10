#pragma once

#include "Location.h"
#include <random>
#include <string>
#include <vector>

// ============================================================================
// TOWN STRUCTURE
// Represents a town containing schools and religious establishments
// ============================================================================

struct Town {
  int id;
  std::string name;
  std::vector<Location> schools;
  std::vector<Location> religiousEstablishments;

  Town() : id(-1) {}

  Town(int townId, int numSchools, int numReligious, int schoolCap,
       int religiousCap)
      : id(townId) {
    name = "Town_" + std::to_string(townId);

    // Create schools
    for (int i = 0; i < numSchools; ++i) {
      int locId = townId * 1000 + i; // Unique ID scheme
      std::string locName = name + "_School_" + std::to_string(i);
      schools.emplace_back(locId, LocationType::SCHOOL, townId, locName,
                           schoolCap);
    }

    // Create religious establishments
    for (int i = 0; i < numReligious; ++i) {
      int locId = townId * 1000 + 100 + i; // Offset to avoid ID collision
      std::string locName = name + "_Religious_" + std::to_string(i);
      religiousEstablishments.emplace_back(
          locId, LocationType::RELIGIOUS_ESTABLISHMENT, townId, locName,
          religiousCap);
    }
  }

  // Get a random school location
  Location *getRandomSchool(std::mt19937 &rng) {
    if (schools.empty())
      return nullptr;
    std::uniform_int_distribution<size_t> dist(0, schools.size() - 1);
    return &schools[dist(rng)];
  }

  // Get a random religious establishment
  Location *getRandomReligious(std::mt19937 &rng) {
    if (religiousEstablishments.empty())
      return nullptr;
    std::uniform_int_distribution<size_t> dist(
        0, religiousEstablishments.size() - 1);
    return &religiousEstablishments[dist(rng)];
  }
};
