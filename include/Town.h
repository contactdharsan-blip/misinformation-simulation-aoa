#pragma once
#include "Demographics.h"
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
  std::vector<Location> workplaces;

  Town() : id(-1) {}

  Town(int townId, int numSchools, int numReligious, int numWorkplaces,
       int schoolCap, int religiousCap, int workplaceCap)
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
    // Ensure every denomination except NONE has at least one building first
    int numReligiousDenoms =
        static_cast<int>(ReligiousDenomination::NUM_DENOMINATIONS) - 1;
    for (int i = 0; i < numReligious; ++i) {
      int locId = townId * 1000 + 100 + i;

      ReligiousDenomination denom;
      if (i < numReligiousDenoms) {
        // One of each first
        denom = static_cast<ReligiousDenomination>(i + 1);
      } else {
        // Cycle through them for any remaining capacity
        denom =
            static_cast<ReligiousDenomination>((i % numReligiousDenoms) + 1);
      }

      std::string denomName;
      switch (denom) {
      case ReligiousDenomination::CATHOLIC:
        denomName = "Catholic";
        break;
      case ReligiousDenomination::EVANGELICAL:
        denomName = "Evangelical";
        break;
      case ReligiousDenomination::MAINLINE:
        denomName = "Mainline";
        break;
      case ReligiousDenomination::LDS:
        denomName = "LDS";
        break;
      case ReligiousDenomination::JEWISH:
        denomName = "Jewish";
        break;
      case ReligiousDenomination::MUSLIM:
        denomName = "Muslim";
        break;
      case ReligiousDenomination::BUDDHIST:
        denomName = "Buddhist";
        break;
      case ReligiousDenomination::HINDU:
        denomName = "Hindu";
        break;
      default:
        denomName = "Other";
        break;
      }

      std::string locName = name + "_" + denomName + "_" + std::to_string(i);
      religiousEstablishments.emplace_back(
          locId, LocationType::RELIGIOUS_ESTABLISHMENT, townId, locName,
          religiousCap, denom);
    }

    // Create workplaces
    for (int i = 0; i < numWorkplaces; ++i) {
      int locId = townId * 1000 + 500 + i;
      std::string locName = name + "_Work_" + std::to_string(i);
      workplaces.emplace_back(locId, LocationType::WORKPLACE, townId, locName,
                              workplaceCap);
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

  // Get a random religious establishment of a specific denomination
  Location *getRandomReligiousOfDenomination(std::mt19937 &rng,
                                             ReligiousDenomination denom) {
    std::vector<Location *> matches;
    for (auto &loc : religiousEstablishments) {
      if (loc.denomination == denom) {
        matches.push_back(&loc);
      }
    }
    if (matches.empty())
      return nullptr;
    std::uniform_int_distribution<size_t> dist(0, matches.size() - 1);
    return matches[dist(rng)];
  }

  // Get a random workplace
  Location *getRandomWorkplace(std::mt19937 &rng) {
    if (workplaces.empty())
      return nullptr;
    std::uniform_int_distribution<size_t> dist(0, workplaces.size() - 1);
    return &workplaces[dist(rng)];
  }
};
