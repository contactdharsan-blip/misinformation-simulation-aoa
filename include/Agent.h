#pragma once
#include "Demographics.h"
#include "SEDPNR.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <random>
#include <vector>

#include "Configuration.h"
#include "SEDPNR.h"

// Note: Configurable parameters are now managed by Configuration::instance()

// Agent class follows...

// ============================================================================
// AGENT CLASS
// ============================================================================

class Agent {
public:
  // Unique identifier
  int id;

  // Demographics
  int age;
  int educationLevel; // 0-5 scale
  int homeTownId;
  int schoolLocationId; // -1 if not applicable
  int religiousLocationId;
  int workplaceLocationId; // -1 if not applicable
  EthnicGroup ethnicity;
  ReligiousDenomination denomination;

  // Derived values
  double credibilityValue; // Calculated from age + education

  // Social network
  std::vector<int> connections; // IDs of connected agents

  // SEDPNR states per claim (claim_id -> state)
  std::map<int, SEDPNRState> claimStates;

  // Time spent in current state per claim (for state transitions)
  std::map<int, int> timeInState;

  // Connection tenure: tracks steps since agent became Propagating while
  // connection stayed Susceptible. Key: connection ID, Value: steps
  std::map<int, int> connectionTenure;

  // Constructor
  Agent(int agentId, int agentAge, int eduLevel, int town, int school,
        int religious, int work, EthnicGroup ethnic,
        ReligiousDenomination denom)
      : id(agentId), age(agentAge), educationLevel(eduLevel), homeTownId(town),
        schoolLocationId(school), religiousLocationId(religious),
        workplaceLocationId(work), ethnicity(ethnic), denomination(denom) {
    credibilityValue = calculateCredibility();
  }

  // Default constructor
  Agent()
      : id(-1), age(0), educationLevel(0), homeTownId(-1), schoolLocationId(-1),
        religiousLocationId(-1), workplaceLocationId(-1),
        ethnicity(EthnicGroup::WHITE),
        denomination(ReligiousDenomination::NONE), credibilityValue(0) {}

  // ========================================================================
  // CREDIBILITY CALCULATION
  // Combines age and education to create a credibility value
  // PLACEHOLDER IMPLEMENTATION - Adjust formula as needed
  // ========================================================================
  double calculateCredibility() const {
    // Normalize education to 0-1 range
    double eduNormalized = static_cast<double>(educationLevel) / 5.0;

    // Age-based credibility (bell curve around optimal age)
    auto &cfg = Configuration::instance();
    double ageFactor = 0.0;
    if (cfg.age_spread > 0) {
      ageFactor = std::exp(-std::pow(age - cfg.age_optimal, 2) /
                           (2 * cfg.age_spread * cfg.age_spread));
    }

    // Combine factors with weights
    double credibility =
        cfg.age_weight * ageFactor + cfg.edu_weight * eduNormalized;

    // Clamp to 0-1 range
    return std::max(0.0, std::min(1.0, credibility));
  }

  // Calculate similarity multiplier with another agent (Base 1.0 + bonuses)
  // Higher value means higher probability of influence/transmission
  double calculateSimilarity(const Agent &other) const {
    double score = 1.0; // Base multiplier

    // Ethnicity match
    if (ethnicity == other.ethnicity)
      score += 0.2; // +20%

    // Religion match
    if (denomination == other.denomination)
      score += 0.2; // +20%

    // Age proximity (within 10 years)
    if (std::abs(age - other.age) <= 10)
      score += 0.1; // +10%

    // Education level (within 1 level)
    if (std::abs(educationLevel - other.educationLevel) <= 1)
      score += 0.1; // +10%

    return score;
  }

  // ========================================================================
  // GET AGE GROUP
  // ========================================================================
  AgeGroup getAgeGroup() const {
    if (age <= 12)
      return AgeGroup::CHILD;
    if (age <= 19)
      return AgeGroup::TEEN;
    if (age <= 35)
      return AgeGroup::YOUNG_ADULT;
    if (age <= 55)
      return AgeGroup::ADULT;
    return AgeGroup::SENIOR;
  }

  // ========================================================================
  // INTERACTION PROBABILITY
  // Calculates probability of interaction between this agent and another
  // Based on location, age group, and ethnic group
  // PLACEHOLDER IMPLEMENTATION - Returns base probability due to placeholder
  // params
  // ========================================================================
  double getInteractionProbability(const Agent &other) const {
    auto &cfg = Configuration::instance();
    double prob = cfg.base_interaction_prob;

    // Location-based interaction
    // Highest weight: same school
    if (schoolLocationId != -1 && schoolLocationId == other.schoolLocationId) {
      prob += cfg.same_school_weight;
    }

    // Second highest: same religious establishment
    if (religiousLocationId == other.religiousLocationId) {
      prob += cfg.same_religious_weight;
    }

    // Workplace-based interaction
    if (workplaceLocationId != -1 &&
        workplaceLocationId == other.workplaceLocationId) {
      prob += cfg.same_workplace_weight;
    }

    // Baseline: same town
    if (homeTownId == other.homeTownId) {
      prob += cfg.same_town_weight;
    }

    // Age group factor - same age group interacts more
    if (getAgeGroup() == other.getAgeGroup()) {
      prob += cfg.age_group_weight;
    }

    // Ethnicity factor - same ethnicity interacts more
    if (ethnicity == other.ethnicity) {
      prob += cfg.ethnicity_weight;
    }

    // Clamp to 0-1 range
    return std::max(0.0, std::min(1.0, prob));
  }

  // ========================================================================
  // CLAIM PASSING FREQUENCY
  // Returns how often this agent passes claims based on demographics
  // PLACEHOLDER IMPLEMENTATION
  // ========================================================================
  double getClaimPassingFrequency() const {
    // Placeholder: frequency based on age group and credibility
    // Young adults might share more, seniors might share less
    double baseFrq = 1.0; // Fixed from placeholder 0.0

    switch (getAgeGroup()) {
    case AgeGroup::CHILD:
      baseFrq *= 0.5; // Children share less
      break;
    case AgeGroup::TEEN:
      baseFrq *= 1.5; // Teens share more
      break;
    case AgeGroup::YOUNG_ADULT:
      baseFrq *= 1.2; // Young adults share somewhat more
      break;
    case AgeGroup::ADULT:
      baseFrq *= 1.0; // Adults share at base rate
      break;
    case AgeGroup::SENIOR:
      baseFrq *= 0.8; // Seniors share less
      break;
    default:
      break;
    }

    return baseFrq;
  }

  // ========================================================================
  // STATE MANAGEMENT
  // ========================================================================

  // Check if agent is already involved with any claim (not susceptible)
  bool isInvolved() const {
    for (auto const &entry : claimStates) {
      SEDPNRState state = entry.second;
      if (state != SEDPNRState::SUSCEPTIBLE) {
        return true;
      }
    }
    return false;
  }

  // Get current state for a claim
  SEDPNRState getState(int claimId) const {
    auto it = claimStates.find(claimId);
    if (it != claimStates.end()) {
      return it->second;
    }
    return SEDPNRState::SUSCEPTIBLE; // Default state
  }

  // Set state for a claim
  void setState(int claimId, SEDPNRState state) {
    if (claimStates.find(claimId) == claimStates.end() ||
        claimStates[claimId] != state) {
      timeInState[claimId] = 0; // Reset time when state changes
    }
    claimStates[claimId] = state;
  }

  // Increment time in current state
  void incrementTimeInState(int claimId) { timeInState[claimId]++; }

  // Get time in current state
  int getTimeInState(int claimId) const {
    auto it = timeInState.find(claimId);
    if (it != timeInState.end()) {
      return it->second;
    }
    return 0;
  }

  // ========================================================================
  // CONNECTION MANAGEMENT (for pruning/rewiring)
  // ========================================================================

  void removeConnection(int connId) {
    connections.erase(
        std::remove(connections.begin(), connections.end(), connId),
        connections.end());
    connectionTenure.erase(connId);
  }

  void addConnection(int connId) {
    // Only add if not already present
    if (std::find(connections.begin(), connections.end(), connId) ==
        connections.end()) {
      connections.push_back(connId);
    }
  }

  void resetConnectionTenure() { connectionTenure.clear(); }

  void incrementConnectionTenure(int connId) { connectionTenure[connId]++; }

  int getConnectionTenure(int connId) const {
    auto it = connectionTenure.find(connId);
    if (it != connectionTenure.end()) {
      return it->second;
    }
    return 0;
  }
};
