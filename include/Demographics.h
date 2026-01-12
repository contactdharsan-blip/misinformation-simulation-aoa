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
// 0-12:  12%
// 13-19: 14%
// 20-39: 32% (Young Adult)
// 40-64: 30% (Adult)
// 65+:   12% (Senior)
static const double PHOENIX_AGE_PROBS[] = {0.12, 0.14, 0.32, 0.30, 0.12};

// Ethnic Breakdown (Approximate)
// White (Non-Hisp): 41%
// Hispanic:        42%
// Black:            7%
// Asian:            4%
// Native American:  2%
// Multiracial:      4%
static const double PHOENIX_ETHNICITY_PROBS[] = {0.41, 0.42, 0.07,
                                                 0.04, 0.02, 0.04};

// Religious Affiliation (Approximate)
// None/Unaffiliated: 26%
// Catholic:          21%
// Evangelical:       18%
// Mainline:          9%
// LDS (Mormon):      5%
// Jewish:            2%
// Muslim:            1%
// Buddhist:          3%
// Hindu:             2%
// Other:             13%
static const double PHOENIX_RELIGION_PROBS[] = {0.26, 0.21, 0.18, 0.09, 0.05,
                                                0.02, 0.01, 0.03, 0.02};
