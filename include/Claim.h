#pragma once

#include <string>

// ============================================================================
// CLAIM CLASS
// Represents an information claim (truth or misinformation)
// ============================================================================

#include "Configuration.h"

// Note: Claim parameters are now managed by the Configuration singleton

class Claim {
public:
  // Unique identifier
  int claimId;

  // Is this misinformation or truth?
  bool isMisinformation;

  // Claim-specific parameters (override defaults if set)
  double spreadRate;        // How fast the claim spreads
  double adoptionThreshold; // Threshold for adoption

  // Descriptive name (optional)
  std::string name;

  // Origin information
  int originAgentId; // ID of the agent who started the claim
  int originTime;    // Time step when claim was introduced

  static Claim createMisinformation(int id, const std::string &name = "") {
    auto &cfg = Configuration::instance();
    Claim c;
    c.claimId = id;
    c.isMisinformation = true;
    c.spreadRate = 1.0; // Default base spread
    c.adoptionThreshold = cfg.misinfo_threshold;
    c.name = name.empty() ? "Misinformation_" + std::to_string(id) : name;
    c.originAgentId = -1;
    c.originTime = 0;
    return c;
  }

  static Claim createTruth(int id, const std::string &name = "") {
    auto &cfg = Configuration::instance();
    Claim c;
    c.claimId = id;
    c.isMisinformation = false;
    c.spreadRate = 1.0; // Default base spread
    c.adoptionThreshold = cfg.truth_threshold;
    c.name = name.empty() ? "Truth_" + std::to_string(id) : name;
    c.originAgentId = -1;
    c.originTime = 0;
    return c;
  }

  // Default constructor
  Claim()
      : claimId(-1), isMisinformation(false), spreadRate(1.0),
        adoptionThreshold(0.5), originAgentId(-1), originTime(0) {}

  // Get type as string
  std::string getTypeString() const {
    return isMisinformation ? "Misinformation" : "Truth";
  }
};
