#pragma once

// ============================================================================
// SEDPNR STATE ENUMERATION
// Susceptible -> Exposed -> Doubtful -> Propagating/Not-spreading -> Recovered
// ============================================================================

enum class SEDPNRState {
  SUSCEPTIBLE = 0, // S: Has not encountered the claim
  EXPOSED,         // E: Has been exposed to the claim
  DOUBTFUL,        // D: Is evaluating the claim
  PROPAGATING,     // P: Is actively spreading the claim
  NOT_SPREADING,   // N: Has adopted but not spreading
  RECOVERED,       // R: Has rejected or recovered from the claim
  NUM_STATES
};

// Note: SEDPNR parameters are now managed by the Configuration singleton

// ============================================================================
// HELPER FUNCTIONS FOR STATE NAMES
// ============================================================================

inline const char *stateToString(SEDPNRState state) {
  switch (state) {
  case SEDPNRState::SUSCEPTIBLE:
    return "Susceptible";
  case SEDPNRState::EXPOSED:
    return "Exposed";
  case SEDPNRState::DOUBTFUL:
    return "Doubtful";
  case SEDPNRState::PROPAGATING:
    return "Propagating";
  case SEDPNRState::NOT_SPREADING:
    return "Not-Spreading";
  case SEDPNRState::RECOVERED:
    return "Recovered";
  default:
    return "Unknown";
  }
}

inline char stateToChar(SEDPNRState state) {
  switch (state) {
  case SEDPNRState::SUSCEPTIBLE:
    return 'S';
  case SEDPNRState::EXPOSED:
    return 'E';
  case SEDPNRState::DOUBTFUL:
    return 'D';
  case SEDPNRState::PROPAGATING:
    return 'P';
  case SEDPNRState::NOT_SPREADING:
    return 'N';
  case SEDPNRState::RECOVERED:
    return 'R';
  default:
    return '?';
  }
}
