#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

// ============================================================================
// UNIFIED CONFIGURATION
// Tracks all simulation parameters in one place
// ============================================================================

struct Configuration {
  // Simulation Core
  int population = 1000;
  int timesteps = 1000;
  unsigned int seed = 42;

  // Town/Location Settings
  int num_towns = 5;
  int schools_per_town = 3;
  int religious_per_town = 5;
  int school_capacity = 200;
  int religious_capacity = 150;
  int workplaces_per_town = 10;
  int workplace_capacity = 500;

  // Agent Demographics & Credibility
  double age_weight = 0.4;
  double edu_weight = 0.6;
  double age_optimal = 45.0;
  double age_spread = 20.0;
  double credibility_rejection_weight = 0.1;

  // Social Network
  int max_connections = 10;
  double base_interaction_prob = 0.05;
  double same_school_weight = 0.5;
  double same_religious_weight = 0.4;
  double same_town_weight = 0.2;
  double age_group_weight = 0.3;
  double ethnicity_weight = 0.2;
  double religious_participation_prob = 0.6; // 60% of population is religious
  double same_workplace_weight = 0.45;
  double homophily_strength = 2.0;

  // SEDPNR Transitions
  double prob_s_to_e = 0.1;
  double prob_e_to_d = 0.2;
  double prob_d_to_p = 0.05;
  double prob_d_to_n = 0.1;
  double prob_d_to_r = 0.05;
  double prob_p_to_n = 0.1;
  double prob_p_to_r = 0.05;
  double prob_n_to_r = 0.05;

  // Claim Mechanics
  double misinfo_multiplier = 6.0;
  double truth_threshold = 0.8;
  double misinfo_threshold = 0.3;

  // Simulation Settings
  int output_interval = 1;
  bool full_spatial_snapshot = true; // Record all agents for visualization

  // Connection Pruning
  bool enable_connection_pruning = true;
  int connection_patience = 50; // Steps before pruning unresponsive connection

  // Singleton access
  static Configuration &instance() {
    static Configuration config;
    return config;
  }

  // Load from .cfg file
  void load(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open())
      return;

    std::string line;
    while (std::getline(file, line)) {
      size_t comment = line.find('#');
      if (comment != std::string::npos)
        line = line.substr(0, comment);
      if (line.empty())
        continue;

      std::stringstream ss(line);
      std::string key, val;
      if (std::getline(ss, key, '=') && std::getline(ss, val)) {
        key = trim(key);
        val = trim(val);
        updateParam(key, val);
      }
    }
    std::cout << "Successfully loaded configuration from " << filename
              << std::endl;
  }

private:
  std::string trim(const std::string &s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
      return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
  }

  void updateParam(const std::string &key, const std::string &val) {
    try {
      if (key == "population")
        population = std::stoi(val);
      else if (key == "timesteps")
        timesteps = std::stoi(val);
      else if (key == "age_weight")
        age_weight = std::stod(val);
      else if (key == "edu_weight")
        edu_weight = std::stod(val);
      else if (key == "num_towns")
        num_towns = std::stoi(val);
      else if (key == "schools_per_town")
        schools_per_town = std::stoi(val);
      else if (key == "religious_per_town")
        religious_per_town = std::stoi(val);
      else if (key == "school_capacity")
        school_capacity = std::stoi(val);
      else if (key == "religious_capacity")
        religious_capacity = std::stoi(val);
      else if (key == "same_school_weight")
        same_school_weight = std::stod(val);
      else if (key == "same_religious_weight")
        same_religious_weight = std::stod(val);
      else if (key == "same_town_weight")
        same_town_weight = std::stod(val);
      else if (key == "max_connections")
        max_connections = std::stoi(val);
      else if (key == "misinfo_multiplier")
        misinfo_multiplier = std::stod(val);
      else if (key == "truth_threshold")
        truth_threshold = std::stod(val);
      else if (key == "misinfo_threshold")
        misinfo_threshold = std::stod(val);
      else if (key == "prob_s_to_e")
        prob_s_to_e = std::stod(val);
      else if (key == "prob_e_to_d")
        prob_e_to_d = std::stod(val);
      else if (key == "prob_d_to_p")
        prob_d_to_p = std::stod(val);
      else if (key == "prob_d_to_n")
        prob_d_to_n = std::stod(val);
      else if (key == "prob_d_to_r")
        prob_d_to_r = std::stod(val);
      else if (key == "prob_p_to_n")
        prob_p_to_n = std::stod(val);
      else if (key == "prob_p_to_r")
        prob_p_to_r = std::stod(val);
      else if (key == "prob_n_to_r")
        prob_n_to_r = std::stod(val);
      else if (key == "seed")
        seed = (unsigned int)std::stoul(val);
      else if (key == "output_interval")
        output_interval = std::stoi(val);
      else if (key == "religious_participation_prob")
        religious_participation_prob = std::stod(val);
      else if (key == "workplaces_per_town")
        workplaces_per_town = std::stoi(val);
      else if (key == "workplace_capacity")
        workplace_capacity = std::stoi(val);
      else if (key == "same_workplace_weight")
        same_workplace_weight = std::stod(val);
      else if (key == "homophily_strength")
        homophily_strength = std::stod(val);
      else if (key == "full_spatial_snapshot")
        full_spatial_snapshot = (val == "true" || val == "1");
      else if (key == "enable_connection_pruning")
        enable_connection_pruning = (val == "true" || val == "1");
      else if (key == "connection_patience")
        connection_patience = std::stoi(val);
    } catch (...) {
    }
  }
};
