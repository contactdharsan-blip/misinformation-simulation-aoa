#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

// ============================================================================
// CONFIG LOADER
// Simple key-value parser for .cfg files
// ============================================================================

class ConfigLoader {
public:
  std::map<std::string, std::string> params;

  bool load(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Warning: Could not open config file " << filename
                << ". Using defaults." << std::endl;
      return false;
    }

    std::string line;
    while (std::getline(file, line)) {
      // Remove comments and whitespace
      size_t commentPos = line.find('#');
      if (commentPos != std::string::npos)
        line = line.substr(0, commentPos);

      if (line.empty())
        continue;

      std::stringstream ss(line);
      std::string key, value;
      if (std::getline(ss, key, '=') && std::getline(ss, value)) {
        // Trim whitespace
        key = trim(key);
        value = trim(value);
        if (!key.empty() && !value.empty()) {
          params[key] = value;
        }
      }
    }
    return true;
  }

  double getDouble(const std::string &key, double defaultValue) const {
    auto it = params.find(key);
    if (it != params.end()) {
      try {
        return std::stod(it->second);
      } catch (...) {
        return defaultValue;
      }
    }
    return defaultValue;
  }

  int getInt(const std::string &key, int defaultValue) const {
    auto it = params.find(key);
    if (it != params.end()) {
      try {
        return std::stoi(it->second);
      } catch (...) {
        return defaultValue;
      }
    }
    return defaultValue;
  }

private:
  std::string trim(const std::string &s) const {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
      return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
  }
};
