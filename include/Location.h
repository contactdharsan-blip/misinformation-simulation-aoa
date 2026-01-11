#pragma once
#include "Demographics.h"
#include <string>
#include <vector>

// ============================================================================
// LOCATION TYPE
// ============================================================================

enum class LocationType {
  SCHOOL,
  RELIGIOUS_ESTABLISHMENT,
  WORKPLACE,
  HOME // For future expansion
};

// ============================================================================
// LOCATION STRUCTURE
// Represents a physical location where agents gather
// ============================================================================

struct Location {
  int id;
  LocationType type;
  int townId;
  std::string name;
  int capacity;                       // Max agents that can be assigned
  ReligiousDenomination denomination; // Only for religious sites
  std::vector<int> assignedAgents;    // Agent IDs at this location

  Location()
      : id(-1), type(LocationType::HOME), townId(-1), capacity(0),
        denomination(ReligiousDenomination::NONE) {}

  Location(int locId, LocationType locType, int town,
           const std::string &locName, int cap,
           ReligiousDenomination denom = ReligiousDenomination::NONE)
      : id(locId), type(locType), townId(town), name(locName), capacity(cap),
        denomination(denom) {}

  // Add an agent to this location
  bool assignAgent(int agentId) {
    if (static_cast<int>(assignedAgents.size()) >= capacity) {
      return false; // Location full
    }
    assignedAgents.push_back(agentId);
    return true;
  }

  // Get location type as string
  std::string getTypeString() const {
    switch (type) {
    case LocationType::SCHOOL:
      return "School";
    case LocationType::RELIGIOUS_ESTABLISHMENT:
      return "Religious";
    case LocationType::WORKPLACE:
      return "Workplace";
    case LocationType::HOME:
      return "Home";
    default:
      return "Unknown";
    }
  }
};
