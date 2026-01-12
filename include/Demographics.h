#pragma once

// ============================================================================
// ETHNIC GROUP ENUMERATION (Phoenix specific)
// ============================================================================

enum class EthnicGroup {
  WHITE = 0,
  HISPANIC,
  BLACK,
  ASIAN,
  NATIVE_AMERICAN,
  MULTIRACIAL,
  NUM_GROUPS
};

// ============================================================================
// AGE GROUP ENUMERATION
// ============================================================================

enum class AgeGroup {
  CHILD = 0,   // 0-12
  TEEN,        // 13-19
  YOUNG_ADULT, // 20-35
  ADULT,       // 36-55
  SENIOR,      // 56+
  NUM_GROUPS
};

// ============================================================================
// RELIGIOUS DENOMINATION ENUMERATION (Phoenix specific)
// ============================================================================

enum class ReligiousDenomination {
  NONE = 0,
  CATHOLIC,
  EVANGELICAL,
  MAINLINE,
  LDS,
  JEWISH,
  MUSLIM,
  BUDDHIST,
  HINDU,
  NUM_DENOMINATIONS
};

// ============================================================================
// DEMOGRAPHIC PROBABILITIES (Phoenix, AZ Estimates ~2024)
// ============================================================================

// Age Distribution (Approximate)
// 0-12:  15%
// 13-19: 10%
// 20-35: 25%
// 36-55: 25%
// 56+:   25%
static const double PHOENIX_AGE_PROBS[] = {0.15, 0.10, 0.25, 0.25, 0.25};

// Ethnic Breakdown (Approximate)
// White (Non-Hisp): 40%
// Hispanic:        42%
// Black:            8%
// Asian:            4%
// Native American:  2%
// Multiracial:      4%
static const double PHOENIX_ETHNICITY_PROBS[] = {0.40, 0.42, 0.08,
                                                 0.04, 0.02, 0.04};

// Religious Affiliation (Approximate)
// None/Unaffiliated: 33%
// Catholic:          21%
// Evangelical:       18%
// Mainline:          12%
// LDS (Mormon):       6%
// Other:             10% (Distributed among remaining)
static const double PHOENIX_RELIGION_PROBS[] = {0.33, 0.21, 0.18, 0.12, 0.06,
                                                0.02, 0.02, 0.03, 0.03};
