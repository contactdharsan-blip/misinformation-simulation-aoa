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

// Simple struct to hold agent spatial state at a point in time
struct Snapshot {
  int agentId;
  int townId;
  int schoolId;
  int religiousId;
  int claimId;
  int state;
  bool isMisinfo;
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
sf::Vector2f getAgentOffset(int agentId) {
  float x = std::fmod(agentId * 123.45f, 30.0f) - 15.0f;
  float y = std::fmod(agentId * 678.90f, 30.0f) - 15.0f;
  return {x, y};
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
  // Maps to track which locations belong to which town for Detailed View
  std::map<int, std::vector<int>> townSchools;
  std::map<int, std::vector<int>> townReligious;
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
      s.townId = cols[2];
      s.schoolId = cols[3];
      s.religiousId = cols[4];
      s.claimId = cols[5];

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
      s.state = cols[6];
      s.isMisinfo = (cols[7] == 1);

      timeline[time].push_back(s);
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
  float cellW = (float)WINDOW_SIZE / (float)numCols;
  float cellH = (float)WINDOW_SIZE / (float)numRows;

  // Playback state
  int currentTime = 0;
  int selectedClaim = 0;
  bool isPlaying = false;
  float playbackSpeed = 1.0f;
  sf::Clock playbackClock;

  ViewMode currentView = GRID_VIEW;
  int currentDistrictId = 0;

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
      }
    }

    // Playback logic
    if (isPlaying &&
        playbackClock.getElapsedTime().asSeconds() > (0.1f / playbackSpeed)) {
      currentTime++;
      if (currentTime > maxTime) {
        currentTime = 0;
        isPlaying = false;
      }
    }

    // Draw Agents
    int propagating = 0;
    int recovered = 0;

    if (timeline.count(currentTime)) {
      for (auto &s : timeline[currentTime]) {
        if (s.claimId != selectedClaim)
          continue;

        // Stats collection
        if (s.state == 3)
          propagating++;
        if (s.state == 4 || s.state == 5)
          recovered++;

        float targetX, targetY; // Declare outside

        if (currentView == GRID_VIEW) {
          int r = s.townId / numCols;
          int c = s.townId % numCols;
          float gridCellW = (float)WINDOW_SIZE / numCols;
          float gridCellH = (float)WINDOW_SIZE / numRows;

          if (s.schoolId != -1 && s.religiousId != -1) {
            targetX = (c * gridCellW) + (gridCellW * 0.5f);
            targetY = (r * gridCellH) + (gridCellH * 0.55f);
          } else if (s.schoolId != -1) {
            targetX = (c * gridCellW) + (gridCellW * 0.25f);
            targetY = (r * gridCellH) + (gridCellH * 0.35f);
          } else if (s.religiousId != -1) {
            targetX = (c * gridCellW) + (gridCellW * 0.75f);
            targetY = (r * gridCellH) + (gridCellH * 0.75f);
          } else {
            targetX = (c * gridCellW) + (gridCellW * 0.5f);
            targetY = (r * gridCellH) + (gridCellH * 0.15f);
          }
        } else {
          // DISTRICT VIEW
          if (s.townId != currentDistrictId)
            continue; // Don't draw agents from other towns

          sf::Vector2f sPos = {0, 0};
          sf::Vector2f rPos = {0, 0};
          bool hasS = (s.schoolId != -1);
          bool hasR = (s.religiousId != -1);

          if (hasS)
            sPos = getLocationCoords(s.schoolId, 12, WINDOW_SIZE);
          if (hasR)
            rPos = getLocationCoords(s.religiousId, 34, WINDOW_SIZE);

          if (hasS && hasR) {
            targetX = (sPos.x + rPos.x) / 2.0f;
            targetY = (sPos.y + rPos.y) / 2.0f;
          } else if (hasS) {
            targetX = sPos.x;
            targetY = sPos.y;
          } else if (hasR) {
            targetX = rPos.x;
            targetY = rPos.y;
          } else {
            // Just random scatter if neither
            targetX = (float)WINDOW_SIZE * 0.5f;
            targetY = (float)WINDOW_SIZE * 0.1f;
          }
        }

        auto offset = getAgentOffset(s.agentId);
        sf::CircleShape agent(2.0f);
        agent.setOrigin({1.0f, 1.0f});
        agent.setPosition({targetX + offset.x, targetY + offset.y});

        sf::Color color(50, 50, 50); // Susceptible
        if (s.state == 3) {
          color = s.isMisinfo ? sf::Color(239, 68, 68) : sf::Color(79, 70, 229);
          agent.setRadius(3.5f);
        } else if (s.state == 1 || s.state == 2) {
          color = sf::Color(245, 158, 11); // Exposed/Doubtful
        } else if (s.state == 4 || s.state == 5) {
          color = sf::Color(16, 185, 129); // Adopted
        }

        agent.setFillColor(color);
        window.draw(agent);
      }
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

      drawStat("ACTIVE PROPAGATORS", std::to_string(propagating), 80,
               sf::Color(239, 68, 68));
      drawStat("RECOVERED / NEUTRAL", std::to_string(recovered), 160,
               sf::Color(16, 185, 129));

      float reachP = (float)recovered / (float)g_config.population * 100.0f;
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
                    "CLAIM: " +
                        (selectedClaim == 0
                             ? std::string("TRUTH")
                             : "MISINFO " + std::to_string(selectedClaim)),
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
      drawLegendItem("Exposed / Doubtful", sf::Color(245, 158, 11),
                     legendY + 70);
      drawLegendItem("Adopted / Neutral", sf::Color(16, 185, 129),
                     legendY + 90);
      drawLegendItem("Susceptible", sf::Color(50, 50, 50), legendY + 110);
      drawLegendItem("School Zone", sf::Color(63, 102, 241), legendY + 140,
                     false);
      drawLegendItem("Religious Zone", sf::Color(168, 85, 247), legendY + 160,
                     false);

      sf::Text help(font,
                    "Space: Play/Pause | R: Reset\nArrows: Seek/Speed | 0-9: "
                    "Claim\nTab: Toggle View | [ ]: Switch District",
                    12);
      help.setPosition({(float)WINDOW_SIZE + 20, 750});
      help.setFillColor(sf::Color(80, 80, 80));
      window.draw(help);
    }

    window.display();
  }

  return 0;
}
