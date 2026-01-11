#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct StateCounts {
  int susceptible = 0;
  int exposed = 0;
  int doubtful = 0;
  int propagating = 0;
  int notSpreading = 0;
  int recovered = 0;
};

// Simple struct to hold agent spatial state at a point in time
struct Snapshot {
  int agentId;
  int townId;
  int schoolId;
  int religiousId;
  int workplaceId;
  int claimId;
  int state;
  bool isMisinfo;
  int ethnicity;
  int denomination;
};

// Global config (mirrors dashboard logic)
struct Config {
  int numTowns = 5;
  int population = 1000;
  int schoolsPerTown = 3;   // Default, overwritten by config
  int religiousPerTown = 5; // Default, overwritten by config
} g_config;

// Load parameters.cfg to get town counts
void loadConfig() {
  std::ifstream file("parameters.cfg");
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);
    try {
      if (key == "num_towns")
        g_config.numTowns = std::max(1, std::stoi(val));
      if (key == "population")
        g_config.population = std::max(1, std::stoi(val));
      if (key == "schools_per_town")
        g_config.schoolsPerTown = std::max(1, std::stoi(val));
      if (key == "religious_per_town")
        g_config.religiousPerTown = std::max(1, std::stoi(val));
    } catch (...) {
      continue;
    }
  }
}

// Pseudo-random jitter for agent offsets (consistent with JS implementation)
// Pseudo-random jitter for agent offsets
sf::Vector2f getAgentOffset(int agentId) {
  float x = std::fmod(agentId * 123.45f, 40.0f) - 20.0f;
  float y = std::fmod(agentId * 678.90f, 40.0f) - 20.0f;
  return {x, y};
}

// Deterministic home position for an agent in their district
sf::Vector2f getAgentHomePos(int agentId, int windowSize) {
  std::mt19937 rng(agentId + 8888);
  std::uniform_real_distribution<float> dist(0.1f, 0.9f);
  return {dist(rng) * windowSize, dist(rng) * windowSize};
}

// Pseudo-random but consistent location coordinate generator for District View
sf::Vector2f getLocationCoords(int locationId, int seed, int windowSize) {
  std::mt19937 rng(seed + locationId * 9876);
  std::uniform_real_distribution<float> dist(0.1f,
                                             0.9f); // Keep away from very edges
  return {dist(rng) * windowSize, dist(rng) * windowSize};
}

enum ViewMode { GRID_VIEW, DISTRICT_VIEW };

int main() {
  loadConfig();

  // SFML Setup
  const int WINDOW_SIZE = 800;
  const int UI_WIDTH = 300;

  sf::VideoMode mode(
      {(unsigned int)(WINDOW_SIZE + UI_WIDTH), (unsigned int)WINDOW_SIZE});
  sf::RenderWindow window(mode, "City Simulation Visualizer");
  window.setFramerateLimit(60);

  // Load Data
  std::ifstream file("output/spatial_data.csv");
  std::string line;
  std::map<int, std::vector<Snapshot>> timeline;
  // townTimeline[time][claimId][townId] -> StateCounts
  std::map<int, std::map<int, std::map<int, StateCounts>>> townTimeline;
  // Maps to track which locations belong to which town for Detailed View
  std::map<int, std::vector<int>> townSchools;
  std::map<int, std::vector<int>> townReligious;
  std::map<int, std::vector<int>> townWorkplaces;
  int maxTime = 0;

  if (file.is_open()) {
    std::getline(file, line); // Skip header
    while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string item;
      std::vector<int> cols;
      while (std::getline(ss, item, ',')) {
        try {
          cols.push_back(std::stoi(item));
        } catch (...) {
          continue;
        }
      }
      if (cols.size() < 8)
        continue;

      Snapshot s;
      int time = cols[0];
      s.agentId = cols[1];
      if (time > maxTime)
        maxTime = time;
      s.townId = cols[2];
      s.schoolId = cols[3];
      s.religiousId = cols[4];
      s.workplaceId = cols[5];
      s.claimId = cols[6];

      s.state = cols[7];
      s.isMisinfo = (cols[8] == 1);
      if (cols.size() >= 11) {
        s.ethnicity = cols[9];
        s.denomination = cols[10];
      } else {
        s.ethnicity = 0;
        s.denomination = 0;
      }

      // Populate town mappings
      if (s.schoolId != -1) {
        bool exists = false;
        for (int sid : townSchools[s.townId])
          if (sid == s.schoolId)
            exists = true;
        if (!exists)
          townSchools[s.townId].push_back(s.schoolId);
      }
      if (s.religiousId != -1) {
        bool exists = false;
        for (int rid : townReligious[s.townId])
          if (rid == s.religiousId)
            exists = true;
        if (!exists)
          townReligious[s.townId].push_back(s.religiousId);
      }
      if (s.workplaceId != -1) {
        bool exists = false;
        for (int wid : townWorkplaces[s.townId])
          if (wid == s.workplaceId)
            exists = true;
        if (!exists)
          townWorkplaces[s.townId].push_back(s.workplaceId);
      }

      timeline[time].push_back(s);

      // Per-town, Per-claim state tracking
      StateCounts &tc = townTimeline[time][s.claimId][s.townId];
      if (s.state == 0)
        tc.susceptible++;
      else if (s.state == 1)
        tc.exposed++;
      else if (s.state == 2)
        tc.doubtful++;
      else if (s.state == 3)
        tc.propagating++;
      else if (s.state == 4)
        tc.notSpreading++;
      else if (s.state == 5)
        tc.recovered++;

      if (time > maxTime)
        maxTime = time;
    }
  } else {
    std::cerr << "Error: Could not open output/spatial_data.csv" << std::endl;
    return 1;
  }

  // Load Font - Try multiple common system paths
  sf::Font font;
  bool fontLoaded = false;
  std::vector<std::string> fontPaths = {
      "/System/Library/Fonts/Helvetica.ttc",
      "/System/Library/Fonts/Cache/Helvetica.ttc",
      "/System/Library/Fonts/Supplemental/Arial.ttf",
      "/Library/Fonts/Arial.ttf"};

  for (const auto &path : fontPaths) {
    if (font.openFromFile(path)) {
      fontLoaded = true;
      break;
    }
  }

  if (!fontLoaded) {
    std::cerr << "Warning: Could not load any system font. UI elements may not "
                 "render."
              << std::endl;
  }

  // Pre-calculate town layout
  int numCols = std::ceil(std::sqrt(g_config.numTowns));
  int numRows = std::ceil((float)g_config.numTowns / (float)numCols);
  // Playback state
  int currentTime = 0;
  int selectedClaim = 0;
  bool isPlaying = true;
  float playbackSpeed = 1.0f;
  sf::Clock playbackClock;

  ViewMode currentView = GRID_VIEW;
  int currentDistrictId = 0;

  // New: Stateful tracking for sparse data
  std::map<int, std::map<int, Snapshot>>
      persistentState; // [claimId][agentId] -> Snapshot
  std::map<int, std::map<int, StateCounts>>
      persistentTownStats; // [claimId][townId] -> counts
  int lastProcessedTime = -1;

  auto updatePersistentState = [&](int targetTime) {
    if (targetTime < lastProcessedTime) {
      persistentState.clear();
      persistentTownStats.clear();
      lastProcessedTime = -1;
    }
    for (int t = lastProcessedTime + 1; t <= targetTime; ++t) {
      if (timeline.count(t)) {
        for (auto &s : timeline[t]) {
          // If we have an existing state, subtract it from town stats
          if (persistentState[s.claimId].count(s.agentId)) {
            Snapshot old = persistentState[s.claimId][s.agentId];
            StateCounts &tc = persistentTownStats[s.claimId][old.townId];
            if (old.state == 0)
              tc.susceptible--;
            else if (old.state == 1)
              tc.exposed--;
            else if (old.state == 2)
              tc.doubtful--;
            else if (old.state == 3)
              tc.propagating--;
            else if (old.state == 4)
              tc.notSpreading--;
            else if (old.state == 5)
              tc.recovered--;
          }

          persistentState[s.claimId][s.agentId] = s;
          StateCounts &tc = persistentTownStats[s.claimId][s.townId];
          if (s.state == 0)
            tc.susceptible++;
          else if (s.state == 1)
            tc.exposed++;
          else if (s.state == 2)
            tc.doubtful++;
          else if (s.state == 3)
            tc.propagating++;
          else if (s.state == 4)
            tc.notSpreading++;
          else if (s.state == 5)
            tc.recovered++;
        }
      }
    }
    lastProcessedTime = targetTime;
  };

  // Initial populate for Time 0
  updatePersistentState(0);

  std::cout << "Loaded " << maxTime << " steps of data. Visualization ready."
            << std::endl;

  while (window.isOpen()) {
    // SFML 3.0 Event Loop
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      } else if (const auto *keyPressed =
                     event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Space) {
          isPlaying = !isPlaying;
          if (isPlaying)
            playbackClock.restart(); // Prevent jump when resuming
        }
        if (keyPressed->code == sf::Keyboard::Key::Tab) {
          if (currentView == GRID_VIEW)
            currentView = DISTRICT_VIEW;
          else
            currentView = GRID_VIEW;
        }
        if (keyPressed->code ==
            sf::Keyboard::Key::LBracket) { // Scroll districts
          if (currentView == DISTRICT_VIEW) {
            currentDistrictId =
                (currentDistrictId - 1 + g_config.numTowns) % g_config.numTowns;
          }
        }
        if (keyPressed->code == sf::Keyboard::Key::RBracket) {
          if (currentView == DISTRICT_VIEW) {
            currentDistrictId = (currentDistrictId + 1) % g_config.numTowns;
          }
        }
        if (keyPressed->code == sf::Keyboard::Key::R) {
          currentTime = 0;
          isPlaying = false;
        }
        if (keyPressed->code == sf::Keyboard::Key::Left)
          currentTime = std::max(0, currentTime - 1);
        if (keyPressed->code == sf::Keyboard::Key::Right)
          currentTime = std::min(maxTime, currentTime + 1);
        if (keyPressed->code == sf::Keyboard::Key::Up)
          playbackSpeed += 0.5f;
        if (keyPressed->code == sf::Keyboard::Key::Down)
          playbackSpeed = std::max(0.1f, playbackSpeed - 0.5f);
        if (keyPressed->code == sf::Keyboard::Key::Num0)
          selectedClaim = 0;
        if (keyPressed->code == sf::Keyboard::Key::Num1)
          selectedClaim = 1;
        if (keyPressed->code == sf::Keyboard::Key::Num2)
          selectedClaim = 2;
        if (keyPressed->code == sf::Keyboard::Key::Num3)
          selectedClaim = 3;
        if (keyPressed->code == sf::Keyboard::Key::Num4)
          selectedClaim = 4;
        if (keyPressed->code == sf::Keyboard::Key::Num5)
          selectedClaim = 5;
        if (keyPressed->code == sf::Keyboard::Key::Num6)
          selectedClaim = 6;
        if (keyPressed->code == sf::Keyboard::Key::Num7)
          selectedClaim = 7;
        if (keyPressed->code == sf::Keyboard::Key::Num8)
          selectedClaim = 8;
        if (keyPressed->code == sf::Keyboard::Key::Num9)
          selectedClaim = 9;
        if (keyPressed->code == sf::Keyboard::Key::A)
          selectedClaim = -1; // SHOW ALL
      } else if (const auto *mouseButtonPressed =
                     event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
          float mouseX = (float)mouseButtonPressed->position.x;
          float mouseY = (float)mouseButtonPressed->position.y;

          // Check for tab clicks (y: 0 to 40)
          if (mouseY >= 0 && mouseY <= 40 && mouseX < 800) {
            float tabWidth = 800.0f / (g_config.numTowns + 1);
            int tabIndex = (int)(mouseX / tabWidth);
            if (tabIndex == 0) {
              currentView = GRID_VIEW;
            } else {
              currentView = DISTRICT_VIEW;
              currentDistrictId = std::min(g_config.numTowns - 1, tabIndex - 1);
            }
          }
        }
      }
    }

    // Playback logic
    if (isPlaying &&
        playbackClock.getElapsedTime().asSeconds() > (0.1f / playbackSpeed)) {
      currentTime++;
      playbackClock.restart();
      if (currentTime > maxTime) {
        currentTime = 0;
        isPlaying = false;
      }
    }

    // Ensure state is updated for current time
    if (currentTime != lastProcessedTime) {
      updatePersistentState(currentTime);
    }

    window.clear(sf::Color(15, 15, 15)); // Slightly off-black

    // 0. Draw Tabs at top
    float tabAreaHeight = 40.0f;
    float tabWidth = (float)WINDOW_SIZE / (g_config.numTowns + 1);
    for (int i = 0; i <= g_config.numTowns; ++i) {
      bool isActive = false;
      std::string label;
      if (i == 0) {
        isActive = (currentView == GRID_VIEW);
        label = "Main City";
      } else {
        isActive =
            (currentView == DISTRICT_VIEW && currentDistrictId == (i - 1));
        label = "Dist " + std::to_string(i - 1);
      }

      sf::RectangleShape tab(sf::Vector2f({tabWidth - 2, tabAreaHeight - 4}));
      tab.setPosition({i * tabWidth + 1, 2});
      tab.setFillColor(isActive ? sf::Color(60, 60, 60)
                                : sf::Color(30, 30, 30));
      tab.setOutlineThickness(isActive ? 1.0f : 0.0f);
      tab.setOutlineColor(sf::Color::White);
      window.draw(tab);

      if (fontLoaded) {
        sf::Text txt(font, label, 12);
        sf::FloatRect bounds = txt.getLocalBounds();
        txt.setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});
        txt.setPosition({i * tabWidth + tabWidth / 2.0f, tabAreaHeight / 2.0f});
        txt.setFillColor(isActive ? sf::Color::White
                                  : sf::Color(150, 150, 150));
        window.draw(txt);
      }
    }

    // Adjust drawing start for grid/districts
    float simAreaYOffset = tabAreaHeight;
    float availableSimHeight = (float)WINDOW_SIZE - simAreaYOffset;

    // 1. Draw Background & Grid/Zones
    if (currentView == GRID_VIEW) {
      for (int i = 0; i < g_config.numTowns; ++i) {
        int r = i / numCols;
        int c = i % numCols;
        float gridCellW = (float)WINDOW_SIZE / numCols;
        float gridCellH = (float)WINDOW_SIZE / numRows;

        // Background prevalence coloring
        if (townTimeline.count(currentTime) &&
            townTimeline[currentTime].count(selectedClaim) &&
            townTimeline[currentTime][selectedClaim].count(i)) {
          const auto &tc = townTimeline[currentTime][selectedClaim][i];
          int total = tc.susceptible + tc.exposed + tc.doubtful +
                      tc.propagating + tc.notSpreading + tc.recovered;
          float prevalence = (total > 0) ? (float)tc.propagating / total : 0.0f;

          float cellScaleY = availableSimHeight / (float)WINDOW_SIZE;
          sf::RectangleShape bg(
              sf::Vector2f({gridCellW - 8, (gridCellH * cellScaleY) - 8}));
          bg.setPosition({c * gridCellW + 4,
                          simAreaYOffset + (r * gridCellH * cellScaleY) + 4});
          bg.setFillColor(
              sf::Color(255, 0, 0, (unsigned char)(prevalence * 180)));
          bg.setOutlineThickness(2.0f);
          bg.setOutlineColor(sf::Color(80, 80, 80));
          window.draw(bg);

          if (fontLoaded) {
            sf::Text lb(font, "DISTRICT " + std::to_string(i), 12);
            lb.setPosition(
                {c * gridCellW + 10,
                 simAreaYOffset + (r * gridCellH * cellScaleY) + 10});
            lb.setFillColor(sf::Color(150, 150, 150, 150));
            window.draw(lb);
          }
        } else {
          // Default empty cell
          float cellScaleY = availableSimHeight / (float)WINDOW_SIZE;
          sf::RectangleShape bg(
              sf::Vector2f({gridCellW - 8, (gridCellH * cellScaleY) - 8}));
          bg.setPosition({c * gridCellW + 4,
                          simAreaYOffset + (r * gridCellH * cellScaleY) + 4});
          bg.setFillColor(sf::Color(30, 30, 30));
          bg.setOutlineThickness(1.0f);
          bg.setOutlineColor(sf::Color(50, 50, 50));
          window.draw(bg);
        }
      }
    } else {
      // DISTRICT VIEW Zones
      if (townSchools.count(currentDistrictId)) {
        for (int sid : townSchools[currentDistrictId]) {
          sf::Vector2f pos = getLocationCoords(sid, 12, WINDOW_SIZE);
          pos.y = simAreaYOffset +
                  (pos.y / (float)WINDOW_SIZE) * availableSimHeight;

          // Randomized influence size per school
          std::mt19937 sRng(sid + 777);
          std::uniform_real_distribution<float> sDist(50.0f, 100.0f);
          float size = sDist(sRng);

          sf::RectangleShape zone(sf::Vector2f({size, size}));
          zone.setOrigin({size / 2.0f, size / 2.0f});
          zone.setPosition(pos);
          zone.setFillColor(sf::Color(63, 102, 241, 50));
          zone.setOutlineThickness(2.0f);
          zone.setOutlineColor(sf::Color(63, 102, 241, 200));
          window.draw(zone);
        }
      }
      if (townReligious.count(currentDistrictId)) {
        for (int rid : townReligious[currentDistrictId]) {
          sf::Vector2f pos = getLocationCoords(rid, 34, WINDOW_SIZE);
          pos.y = simAreaYOffset +
                  (pos.y / (float)WINDOW_SIZE) * availableSimHeight;

          // Randomized influence radius per site
          std::mt19937 rRng(rid + 999);
          std::uniform_real_distribution<float> rDist(30.0f, 80.0f);
          float radius = rDist(rRng);

          sf::CircleShape zone(radius);
          zone.setOrigin({radius, radius});
          zone.setPosition(pos);
          zone.setFillColor(sf::Color(168, 85, 247, 50));
          zone.setOutlineThickness(2.0f);
          zone.setOutlineColor(sf::Color(168, 85, 247, 200));
          window.draw(zone);
        }
      }

      if (townWorkplaces.count(currentDistrictId)) {
        for (int wid : townWorkplaces[currentDistrictId]) {
          sf::Vector2f pos = getLocationCoords(wid, 56, WINDOW_SIZE);
          pos.y = simAreaYOffset +
                  (pos.y / (float)WINDOW_SIZE) * availableSimHeight;

          sf::ConvexShape zone;
          zone.setPointCount(6);
          float r = 40.0f;
          for (int i = 0; i < 6; ++i) {
            float angle = i * 2 * 3.14159f / 6;
            zone.setPoint(i, {r * cos(angle), r * sin(angle)});
          }
          zone.setPosition(pos);
          zone.setFillColor(sf::Color(245, 158, 11, 40)); // Amber/Orange
          zone.setOutlineThickness(2.0f);
          zone.setOutlineColor(sf::Color(245, 158, 11, 150));
          window.draw(zone);
        }
      }

      if (fontLoaded) {
        sf::Text distLabel(font,
                           "DISTRICT " + std::to_string(currentDistrictId), 24);
        distLabel.setPosition({20, 20});
        distLabel.setFillColor(sf::Color(200, 200, 200, 100));
        window.draw(distLabel);
      }
    }

    // 2. Draw Agents
    // Use persistentState instead of timeline[currentTime] to support sparse
    // data
    std::vector<Snapshot> toDraw;
    if (selectedClaim == -1) {
      // "Show All" Mode: find best state across all claims for each agent
      std::map<int, Snapshot> bestByAgent;
      for (auto const &[cid, agents] : persistentState) {
        for (auto const &[aid, s] : agents) {
          if (!bestByAgent.count(aid)) {
            bestByAgent[aid] = s;
          } else {
            int oldState = bestByAgent[aid].state;
            int newState = s.state;
            // P(3) > R(5) > N(4) > D(2) > E(1) > S(0)
            auto getRank = [](int st) {
              if (st == 3)
                return 5;
              if (st == 5 || st == 4)
                return 4;
              if (st == 2)
                return 3;
              if (st == 1)
                return 2;
              return 1;
            };
            if (getRank(newState) > getRank(oldState)) {
              bestByAgent[aid] = s;
            } else if (getRank(newState) == getRank(oldState) && s.isMisinfo) {
              bestByAgent[aid] = s;
            }
          }
        }
      }
      for (auto const &[id, s] : bestByAgent)
        toDraw.push_back(s);
    } else {
      // Single Claim Mode
      if (persistentState.count(selectedClaim)) {
        for (auto const &[aid, s] : persistentState.at(selectedClaim)) {
          toDraw.push_back(s);
        }
      }
    }

    for (auto &s : toDraw) {
      float targetX, targetY;

      if (currentView == GRID_VIEW) {
        int r = s.townId / numCols;
        int c = s.townId % numCols;
        float gridCellW = (float)WINDOW_SIZE / numCols;
        float gridCellH = (float)WINDOW_SIZE / numRows;
        float cellScaleY = availableSimHeight / (float)WINDOW_SIZE;

        if (s.schoolId != -1 && s.religiousId != -1) {
          targetX = (c * gridCellW) + (gridCellW * 0.5f);
          targetY = simAreaYOffset + (r * gridCellH * cellScaleY) +
                    (gridCellH * cellScaleY * 0.55f);
        } else if (s.schoolId != -1) {
          targetX = (c * gridCellW) + (gridCellW * 0.25f);
          targetY = simAreaYOffset + (r * gridCellH * cellScaleY) +
                    (gridCellH * cellScaleY * 0.35f);
        } else if (s.religiousId != -1) {
          targetX = (c * gridCellW) + (gridCellW * 0.75f);
          targetY = simAreaYOffset + (r * gridCellH * cellScaleY) +
                    (gridCellH * cellScaleY * 0.75f);
        } else {
          targetX = (c * gridCellW) + (gridCellW * 0.5f);
          targetY = simAreaYOffset + (r * gridCellH * cellScaleY) +
                    (gridCellH * cellScaleY * 0.15f);
        }
      } else {
        // DISTRICT VIEW
        if (s.townId != currentDistrictId)
          continue;

        sf::Vector2f homePos = getAgentHomePos(s.agentId, WINDOW_SIZE);
        sf::Vector2f sPos = {0, 0};
        sf::Vector2f rPos = {0, 0};
        sf::Vector2f wPos = {0, 0};
        bool hasS = (s.schoolId != -1);
        bool hasR = (s.religiousId != -1);
        bool hasW = (s.workplaceId != -1);
        float simScaleY = availableSimHeight / (float)WINDOW_SIZE;

        if (hasS)
          sPos = getLocationCoords(s.schoolId, 12, WINDOW_SIZE);
        if (hasR)
          rPos = getLocationCoords(s.religiousId, 34, WINDOW_SIZE);
        if (hasW)
          wPos = getLocationCoords(s.workplaceId, 56, WINDOW_SIZE);

        // Blend position: 50% Home, 50% Community hubs
        float hWeight = 0.5f;
        float cWeight = 0.5f;

        float tx = hWeight * homePos.x;
        float ty = hWeight * homePos.y;

        int activeHubs = (hasS ? 1 : 0) + (hasR ? 1 : 0) + (hasW ? 1 : 0);
        if (activeHubs > 0) {
          float perHub = cWeight / activeHubs;
          if (hasS) {
            tx += perHub * sPos.x;
            ty += perHub * sPos.y;
          }
          if (hasR) {
            tx += perHub * rPos.x;
            ty += perHub * rPos.y;
          }
          if (hasW) {
            tx += perHub * wPos.x;
            ty += perHub * wPos.y;
          }
          targetX = tx;
          targetY = ty;
        } else {
          targetX = homePos.x;
          targetY = homePos.y;
        }
        targetY = simAreaYOffset + targetY * simScaleY;
      }

      auto offset = getAgentOffset(s.agentId);
      float radius = (currentView == GRID_VIEW) ? 1.0f : 1.5f;
      sf::CircleShape agent(radius);
      agent.setOrigin({radius, radius});
      agent.setPosition({targetX + offset.x, targetY + offset.y});

      sf::Color color(50, 50, 50, 180); // Susceptible (slight alpha)
      if (s.state == 3) {
        color = s.isMisinfo ? sf::Color(239, 68, 68, 220)
                            : sf::Color(79, 70, 229, 220);
        agent.setRadius(radius * 1.5f);
      } else if (s.state == 1) {
        color = sf::Color(245, 158, 11, 200); // Exposed (Orange)
      } else if (s.state == 2) {
        color = sf::Color(250, 204, 21, 200); // Doubtful (Yellow/Gold)
      } else if (s.state == 4 || s.state == 5) {
        color = sf::Color(16, 185, 129, 200);
      }

      agent.setFillColor(color);
      window.draw(agent);
    }

    // UI Panel
    sf::RectangleShape panel(
        sf::Vector2f({(float)UI_WIDTH, (float)WINDOW_SIZE}));
    panel.setPosition({(float)WINDOW_SIZE, 0});
    panel.setFillColor(sf::Color(20, 20, 20));
    panel.setOutlineThickness(1);
    panel.setOutlineColor(sf::Color(40, 40, 40));
    window.draw(panel);

    if (fontLoaded) {
      sf::Text title(font, "SIMULATION ANALYTICS", 18);
      title.setPosition({(float)WINDOW_SIZE + 20, 30});
      title.setFillColor(sf::Color::White);
      window.draw(title);

      auto drawStat = [&](std::string labelStr, std::string valStr, float y,
                          sf::Color valColor) {
        sf::Text lb(font, labelStr, 14);
        lb.setPosition({(float)WINDOW_SIZE + 20, y});
        lb.setFillColor(sf::Color(120, 120, 120));
        window.draw(lb);

        sf::Text val(font, valStr, 32);
        val.setPosition({(float)WINDOW_SIZE + 20, y + 25});
        val.setFillColor(valColor);
        window.draw(val);
      };

      StateCounts activeSC;
      if (currentView == DISTRICT_VIEW) {
        if (selectedClaim == -1) {
          // Sum across all claims for this district from persistent stats
          for (auto const &[cid, towns] : persistentTownStats) {
            if (towns.count(currentDistrictId)) {
              const auto &tc = towns.at(currentDistrictId);
              activeSC.propagating += tc.propagating;
              activeSC.recovered += tc.recovered;
              activeSC.susceptible += tc.susceptible;
              activeSC.exposed += tc.exposed;
              activeSC.doubtful += tc.doubtful;
              activeSC.notSpreading += tc.notSpreading;
            }
          }
        } else if (persistentTownStats.count(selectedClaim) &&
                   persistentTownStats[selectedClaim].count(
                       currentDistrictId)) {
          activeSC = persistentTownStats[selectedClaim][currentDistrictId];
        }
      } else {
        // Main City View: sum across all relevant towns
        if (selectedClaim == -1) {
          for (auto const &[cid, towns] : persistentTownStats) {
            for (auto const &[tid, tc] : towns) {
              activeSC.propagating += tc.propagating;
              activeSC.recovered += tc.recovered;
              activeSC.susceptible += tc.susceptible;
              activeSC.exposed += tc.exposed;
              activeSC.doubtful += tc.doubtful;
              activeSC.notSpreading += tc.notSpreading;
            }
          }
        } else if (persistentTownStats.count(selectedClaim)) {
          for (auto const &[tid, tc] : persistentTownStats[selectedClaim]) {
            activeSC.propagating += tc.propagating;
            activeSC.recovered += tc.recovered;
            activeSC.susceptible += tc.susceptible;
            activeSC.exposed += tc.exposed;
            activeSC.doubtful += tc.doubtful;
            activeSC.notSpreading += tc.notSpreading;
          }
        }
      }

      int propagating = activeSC.propagating;
      int recovered = activeSC.recovered;
      int totalPop = activeSC.susceptible + activeSC.exposed +
                     activeSC.doubtful + activeSC.propagating +
                     activeSC.notSpreading + activeSC.recovered;

      drawStat("ACTIVE PROPAGATORS", std::to_string(propagating), 80,
               sf::Color(239, 68, 68));
      drawStat("RECOVERED / NEUTRAL", std::to_string(recovered), 160,
               sf::Color(16, 185, 129));

      float reachP =
          (totalPop > 0) ? (float)recovered / (float)totalPop * 100.0f : 0.0f;
      char reachBuf[32];
      std::snprintf(reachBuf, sizeof(reachBuf), "%.1f%%", reachP);
      drawStat("TOTAL REACH", reachBuf, 240, sf::Color(79, 70, 229));

      sf::Text timeLabel(font,
                         "Time: " + std::to_string(currentTime) + " / " +
                             std::to_string(maxTime),
                         16);
      timeLabel.setPosition({(float)WINDOW_SIZE + 20, 330});
      timeLabel.setFillColor(sf::Color::White);
      window.draw(timeLabel);

      sf::Text info(font,
                    "CLAIM: " + (selectedClaim == -1
                                     ? std::string("ALL CLAIMS (COMBINED)")
                                     : (selectedClaim == 0
                                            ? std::string("TRUTH")
                                            : "MISINFO " + std::to_string(
                                                               selectedClaim))),
                    14);
      info.setPosition({(float)WINDOW_SIZE + 20, 360});
      info.setFillColor(sf::Color::White);
      window.draw(info);

      // --- Legend Section ---
      float legendY = 420.0f;
      sf::Text legendTitle(font, "LEGEND", 16);
      legendTitle.setPosition({(float)WINDOW_SIZE + 20, legendY});
      legendTitle.setFillColor(sf::Color(150, 150, 150));
      window.draw(legendTitle);

      auto drawLegendItem = [&](std::string label, sf::Color color, float y,
                                bool circle = true) {
        if (circle) {
          sf::CircleShape dot(5.0f);
          dot.setPosition({(float)WINDOW_SIZE + 20, y + 5});
          dot.setFillColor(color);
          window.draw(dot);
        } else {
          sf::RectangleShape box(sf::Vector2f({10.0f, 10.0f}));
          box.setPosition({(float)WINDOW_SIZE + 20, y + 5});
          box.setFillColor(sf::Color::Transparent);
          box.setOutlineThickness(1);
          box.setOutlineColor(color);
          window.draw(box);
        }
        sf::Text txt(font, label, 12);
        txt.setPosition({(float)WINDOW_SIZE + 40, y + 2});
        txt.setFillColor(sf::Color(180, 180, 180));
        window.draw(txt);
      };

      drawLegendItem("Misinfo (Propagating)", sf::Color(239, 68, 68),
                     legendY + 30);
      drawLegendItem("Truth (Propagating)", sf::Color(79, 70, 229),
                     legendY + 50);
      drawLegendItem("Exposed (Aware)", sf::Color(245, 158, 11), legendY + 70);
      drawLegendItem("Doubtful (Evaluating)", sf::Color(250, 204, 21),
                     legendY + 90);
      drawLegendItem("Adopted / Neutral", sf::Color(16, 185, 129),
                     legendY + 110);
      drawLegendItem("Susceptible", sf::Color(50, 50, 50), legendY + 130);
      drawLegendItem("School Zone", sf::Color(63, 102, 241), legendY + 160,
                     false);
      drawLegendItem("Religious Zone", sf::Color(168, 85, 247), legendY + 180,
                     false);

      sf::Text help(
          font,
          "Space: Play/Pause | R: Reset\nArrows: Seek/Speed | 0-9: "
          "Claim\nTab: Toggle View | [ ]: Switch Dist | A: All Claims",
          12);
      help.setPosition({(float)WINDOW_SIZE + 20, 750});
      help.setFillColor(sf::Color(80, 80, 80));
      window.draw(help);
    }

    window.display();
  }

  return 0;
}
