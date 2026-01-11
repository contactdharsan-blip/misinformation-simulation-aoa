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
