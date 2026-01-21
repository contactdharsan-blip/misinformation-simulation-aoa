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
  int seed = 42;
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
      if (key == "religious_per_town")
        g_config.religiousPerTown = std::max(1, std::stoi(val));
      if (key == "seed")
        g_config.seed = std::stoi(val);
    } catch (...) {
      continue;
    }
  }
}

// Pseudo-random jitter for agent offsets
sf::Vector2f getAgentOffset(int agentId) {
  float x = std::fmod(agentId * 123.45f, 40.0f) - 20.0f;
  float y = std::fmod(agentId * 678.90f, 40.0f) - 20.0f;
  return {x, y};
}

// Deterministic home position for an agent in their district
sf::Vector2f getAgentHomePos(int agentId, int windowSize) {
  std::mt19937 rng(agentId + 8888 + g_config.seed);
  std::uniform_real_distribution<float> dist(0.1f, 0.9f);
  return {dist(rng) * windowSize, dist(rng) * windowSize};
}

// Pseudo-random but consistent location coordinate generator
sf::Vector2f getLocationCoords(int locationId, int salt, int windowSize) {
  std::mt19937 rng(salt + locationId * 9876 + g_config.seed);
  std::uniform_real_distribution<float> dist(0.1f, 0.9f);
  return {dist(rng) * windowSize, dist(rng) * windowSize};
}

// Deterministic grid position for school hubs
sf::Vector2f getSchoolGridCoords(int schoolId, int totalSchools,
                                 int windowSize) {
  if (totalSchools <= 0)
    return {windowSize / 2.0f, windowSize / 2.0f};

  int cols = std::ceil(std::sqrt(totalSchools));
  int rows = std::ceil((float)totalSchools / (float)cols);

  int r = schoolId / cols;
  int c = schoolId % cols;

  float cellW = (float)windowSize / cols;
  float cellH = (float)windowSize / rows;

  return {c * cellW + cellW * 0.5f, r * cellH + cellH * 0.5f};
}

enum ViewMode { DISTRICT_VIEW, CHART_VIEW };

int main() {
  loadConfig();

  const int WINDOW_SIZE = 800;
  const int UI_WIDTH = 300;

  sf::VideoMode mode(
      {(unsigned int)(WINDOW_SIZE + UI_WIDTH), (unsigned int)WINDOW_SIZE});
  sf::RenderWindow window(mode, "City Simulation Visualizer");
  window.setFramerateLimit(60);

  std::ifstream file("output/spatial_data.csv");
  std::string line;
  std::map<int, std::vector<Snapshot>> timeline;
  std::map<int, std::map<int, std::map<int, StateCounts>>> townTimeline;
  std::map<int, std::vector<int>> townSchools;
  std::map<int, std::vector<int>> townReligious;
  std::map<int, std::vector<int>> townWorkplaces;
  std::map<int, std::map<int, int>>
      overallTrends; // [time][claimId] -> Adoption count
  int maxTime = 0;

  if (file.is_open()) {
    std::getline(file, line);
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
      }
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

      // Track trends (Adoption = P, N, or R)
      if (s.state >= 3) {
        overallTrends[time][s.claimId]++;
      }
    }
  }

  sf::Font font;
  bool fontLoaded = false;
  std::vector<std::string> fontPaths = {"/System/Library/Fonts/Helvetica.ttc",
                                        "/Library/Fonts/Arial.ttf"};
  for (const auto &path : fontPaths) {
    if (font.openFromFile(path)) {
      fontLoaded = true;
      break;
    }
  }

  int currentTime = 0;
  int selectedClaim = -1;
  bool isPlaying = true;
  float playbackSpeed = 1.0f;
  sf::Clock playbackClock;
  ViewMode currentView = DISTRICT_VIEW;
  int currentDistrictId = 0;

  std::map<int, std::map<int, Snapshot>> persistentState;
  std::map<int, std::map<int, StateCounts>> persistentTownStats;
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

  updatePersistentState(0);

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>())
        window.close();
      else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Space)
          isPlaying = !isPlaying;
        if (keyPressed->code == sf::Keyboard::Key::R) {
          currentTime = 0;
          isPlaying = false;
        }
        if (keyPressed->code == sf::Keyboard::Key::Left)
          currentTime = std::max(0, currentTime - 1);
        if (keyPressed->code == sf::Keyboard::Key::Right)
          currentTime = std::min(maxTime, currentTime + 1);
      } else if (const auto *mouseButtonPressed =
                     event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
          float mouseX = (float)mouseButtonPressed->position.x;
          float mouseY = (float)mouseButtonPressed->position.y;
          if (mouseY >= 0 && mouseY <= 40 && mouseX < 800) {
            float tabWidth = 800.0f / 4.0f;
            int tabIndex = (int)(mouseX / tabWidth);
            if (tabIndex == 0) {
              selectedClaim = -1;
              currentView = DISTRICT_VIEW;
            } else if (tabIndex == 1) {
              selectedClaim = 0;
              currentView = DISTRICT_VIEW;
            } else if (tabIndex == 2) {
              selectedClaim = 1;
              currentView = DISTRICT_VIEW;
            } else if (tabIndex == 3) {
              currentView = CHART_VIEW;
            }
          }
        }
      }
    }

    if (isPlaying &&
        playbackClock.getElapsedTime().asSeconds() > (0.1f / playbackSpeed)) {
      currentTime++;
      playbackClock.restart();
      if (currentTime > maxTime) {
        currentTime = 0;
        isPlaying = false;
      }
    }
    if (currentTime != lastProcessedTime)
      updatePersistentState(currentTime);

    window.clear(sf::Color(15, 15, 15));

    float tabAreaHeight = 40.0f;
    float tabWidth = (float)WINDOW_SIZE / 4.0f;
    for (int i = 0; i < 4; ++i) {
      bool isActive = false;
      if (currentView == CHART_VIEW) {
        isActive = (i == 3);
      } else {
        isActive = (i == 0 && selectedClaim == -1) ||
                   (i == 1 && selectedClaim == 0) ||
                   (i == 2 && selectedClaim >= 1);
      }

      sf::RectangleShape tab(sf::Vector2f({tabWidth - 2, tabAreaHeight - 4}));
      tab.setPosition({i * tabWidth + 1, 2});
      tab.setFillColor(isActive ? sf::Color(60, 60, 60)
                                : sf::Color(30, 30, 30));
      window.draw(tab);
      if (fontLoaded) {
        std::string label =
            (i == 0 ? "OVERVIEW"
                    : (i == 1 ? "TRUTH" : (i == 2 ? "MISINFO" : "TRENDS")));
        sf::Text txt(font, label, 12);
        sf::FloatRect bounds = txt.getLocalBounds();
        txt.setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});
        txt.setPosition({i * tabWidth + tabWidth / 2.0f, tabAreaHeight / 2.0f});
        txt.setFillColor(isActive ? sf::Color::White
                                  : sf::Color(150, 150, 150));
        window.draw(txt);
      }
    }

    float simAreaYOffset = tabAreaHeight;
    float availableSimHeight = (float)WINDOW_SIZE - simAreaYOffset;

    // Background Zones
    if (currentView != CHART_VIEW) {
      if (townReligious.count(currentDistrictId)) {
        for (int rid : townReligious[currentDistrictId]) {
          sf::Vector2f pos = getLocationCoords(rid, 34, WINDOW_SIZE);
          pos.y = simAreaYOffset +
                  (pos.y / (float)WINDOW_SIZE) * availableSimHeight;

          // Variable large radius for ~70% total coverage
          std::mt19937 rRng(rid * 111 + 555 + g_config.seed);
          std::uniform_real_distribution<float> rDist(80.0f, 130.0f);
          float radius = rDist(rRng);

          sf::CircleShape zone(radius);
          zone.setOrigin({radius, radius});
          zone.setPosition(pos);
          zone.setFillColor(sf::Color(
              168, 85, 247, 40)); // slightly more transparent for overlap
          zone.setOutlineThickness(2.0f);
          zone.setOutlineColor(sf::Color(168, 85, 247, 180));
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

          // Variable large size
          std::mt19937 wRng(wid * 222 + 888 + g_config.seed);
          std::uniform_real_distribution<float> wDist(80.0f, 120.0f);
          float r = wDist(wRng);

          for (int i = 0; i < 6; ++i)
            zone.setPoint(
                i, {r * (float)cos(i * 1.047f), r * (float)sin(i * 1.047f)});
          zone.setPosition(pos);
          zone.setFillColor(sf::Color(245, 158, 11, 30)); // overlapping amber
          zone.setOutlineThickness(2.0f);
          zone.setOutlineColor(sf::Color(245, 158, 11, 150));
          window.draw(zone);
        }
      }
    }

    std::vector<Snapshot> toDraw;
    if (selectedClaim == -1) {
      std::map<int, Snapshot> bestByAgent;
      for (auto const &entry : persistentState) {
        const auto &agents = entry.second;
        for (auto const &entry2 : agents) {
          int aid = entry2.first;
          const auto &s = entry2.second;
          if (!bestByAgent.count(aid))
            bestByAgent[aid] = s;
          else if (s.state == 3 ||
                   (s.state != 0 && bestByAgent[aid].state == 0))
            bestByAgent[aid] = s;
        }
      }
      for (auto const &entry : bestByAgent) {
        const auto &s = entry.second;
        toDraw.push_back(s);
      }
    } else if (persistentState.count(selectedClaim)) {
      for (auto const &entry : persistentState.at(selectedClaim)) {
        const auto &s = entry.second;
        toDraw.push_back(s);
      }
    }

    if (currentView == CHART_VIEW) {
      // Draw Adoption Trends Graph
      float graphX = 50.0f;
      float graphY = simAreaYOffset + 50.0f;
      float graphW = WINDOW_SIZE - 100.0f;
      float graphH = availableSimHeight - 120.0f;

      // Draw Axes
      sf::Vertex axes[] = {
          sf::Vertex{sf::Vector2f(graphX, graphY), sf::Color(100, 100, 100)},
          sf::Vertex{sf::Vector2f(graphX, graphY + graphH),
                     sf::Color(100, 100, 100)},
          sf::Vertex{sf::Vector2f(graphX, graphY + graphH),
                     sf::Color(100, 100, 100)},
          sf::Vertex{sf::Vector2f(graphX + graphW, graphY + graphH),
                     sf::Color(100, 100, 100)}};
      window.draw(axes, 4, sf::PrimitiveType::Lines);

      if (maxTime > 0) {
        // Use total population for scaling to show proportion
        float totalPop = (float)g_config.population;
        if (totalPop <= 0)
          totalPop = 1.0f;

        // Draw Lines for each claim
        // Factual = Blue (0), Misinfo = Red (1)
        std::map<int, sf::Color> claimColors = {{0, sf::Color::Blue},
                                                {1, sf::Color::Red}};

        for (int cid = 0; cid < 2; ++cid) {
          sf::Color col =
              claimColors.count(cid) ? claimColors[cid] : sf::Color::White;
          std::vector<sf::Vertex> line;
          for (int t = 0; t <= currentTime; ++t) {
            int adopted = overallTrends.count(t) && overallTrends[t].count(cid)
                              ? overallTrends[t][cid]
                              : 0;
            float px = graphX + ((float)t / maxTime) * graphW;
            float py = graphY + graphH - ((float)adopted / totalPop) * graphH;
            line.push_back(sf::Vertex{sf::Vector2f(px, py), col});
          }
          if (line.size() > 1) {
            window.draw(line.data(), line.size(), sf::PrimitiveType::LineStrip);
          }
        }
      }

      if (fontLoaded) {
        sf::Text xLabel(font, "Simulation Time (Steps)", 14);
        xLabel.setPosition({graphX + graphW / 2.0f - 80, graphY + graphH + 20});
        xLabel.setFillColor(sf::Color(150, 150, 150));
        window.draw(xLabel);

        sf::Text yLabel(font, "Adoption Proportion (0.0 - 1.0)", 14);
        yLabel.setRotation(sf::degrees(-90.0f));
        yLabel.setPosition({graphX - 35, graphY + graphH / 2.0f + 80});
        yLabel.setFillColor(sf::Color(150, 150, 150));
        window.draw(yLabel);
      }
    } else {
      // Draw Agents
      for (auto &s : toDraw) {
        if (s.townId != currentDistrictId)
          continue;
        sf::Vector2f homePos = getAgentHomePos(s.agentId, WINDOW_SIZE);
        sf::Vector2f sPos = {0, 0}, rPos = {0, 0}, wPos = {0, 0};
        bool hasS = (s.schoolId != -1), hasR = (s.religiousId != -1),
             hasW = (s.workplaceId != -1);
        if (hasS) {
          auto &v = townSchools[s.townId];
          int sIdx = 0;
          for (size_t k = 0; k < v.size(); ++k)
            if (v[k] == s.schoolId) {
              sIdx = k;
              break;
            }
          sPos = getSchoolGridCoords(sIdx, v.size(), WINDOW_SIZE);
        }
        if (hasR)
          rPos = getLocationCoords(s.religiousId, 34, WINDOW_SIZE);
        if (hasW)
          wPos = getLocationCoords(s.workplaceId, 56, WINDOW_SIZE);

        float tx = 0.5f * homePos.x, ty = 0.5f * homePos.y;
        int hubs = (hasS ? 1 : 0) + (hasR ? 1 : 0) + (hasW ? 1 : 0);
        if (hubs > 0) {
          // Choose ONE hub deterministically based on agent ID
          std::vector<sf::Vector2f> availableHubs;
          if (hasS)
            availableHubs.push_back(sPos);
          if (hasR)
            availableHubs.push_back(rPos);
          if (hasW)
            availableHubs.push_back(wPos);

          int chosenIndex = s.agentId % availableHubs.size();
          sf::Vector2f chosenHub = availableHubs[chosenIndex];

          // 50% Home, 50% Selected Hub
          tx += 0.5f * chosenHub.x;
          ty += 0.5f * chosenHub.y;
        } else {
          tx = homePos.x;
          ty = homePos.y;
        }

        ty = simAreaYOffset + ty * (availableSimHeight / (float)WINDOW_SIZE);
        auto offset = getAgentOffset(s.agentId);
        sf::CircleShape agent(1.5f);
        agent.setPosition({tx + offset.x, ty + offset.y});
        sf::Color color(50, 50, 50, 180);
        if (s.state == 3)
          color = s.isMisinfo ? sf::Color::Red : sf::Color::Blue;
        else if (s.state == 1 || s.state == 2)
          color = sf::Color::Yellow;
        else if (s.state >= 4)
          color = sf::Color::Green;
        agent.setFillColor(color);
        window.draw(agent);
      }
    }

    // UI Panel (Side Analytics & Legend)
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
      if (selectedClaim == -1) {
        for (auto const &entry : persistentTownStats) {
          const auto &towns = entry.second;
          for (auto const &entry2 : towns) {
            const auto &tc = entry2.second;
            activeSC.propagating += tc.propagating;
            activeSC.recovered += tc.recovered;
            activeSC.susceptible += tc.susceptible;
            activeSC.exposed += tc.exposed;
            activeSC.doubtful += tc.doubtful;
            activeSC.notSpreading += tc.notSpreading;
          }
        }
      } else if (persistentTownStats.count(selectedClaim)) {
        for (auto const &entry : persistentTownStats[selectedClaim]) {
          const auto &tc = entry.second;
          activeSC.propagating += tc.propagating;
          activeSC.recovered += tc.recovered;
          activeSC.susceptible += tc.susceptible;
          activeSC.exposed += tc.exposed;
          activeSC.doubtful += tc.doubtful;
          activeSC.notSpreading += tc.notSpreading;
        }
      }

      int propagating = activeSC.propagating;
      int adopted = activeSC.notSpreading + activeSC.recovered;
      int totalPop = activeSC.susceptible + activeSC.exposed +
                     activeSC.doubtful + activeSC.propagating +
                     activeSC.notSpreading + activeSC.recovered;

      drawStat("ACTIVE PROPAGATORS", std::to_string(propagating), 80,
               sf::Color(239, 68, 68));
      drawStat("ADOPTED / NEUTRAL", std::to_string(adopted), 160,
               sf::Color(16, 185, 129));

      float reachP =
          (totalPop > 0) ? (float)adopted / (float)totalPop * 100.0f : 0.0f;
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

      // --- Legend Section ---
      float legendY = 400.0f;
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

      if (currentView == CHART_VIEW) {
        drawLegendItem("Factual Claim (Truth)", sf::Color::Blue, legendY + 30);
        drawLegendItem("Misinfo Claim 1", sf::Color::Red, legendY + 50);
      } else {
        drawLegendItem("Misinfo (Propagating)", sf::Color::Red, legendY + 30);
        drawLegendItem("Truth (Propagating)", sf::Color::Blue, legendY + 50);
        drawLegendItem("Exposed / Doubtful", sf::Color::Yellow, legendY + 70);
        drawLegendItem("Adopted / Neutral", sf::Color::Green, legendY + 90);
        drawLegendItem("Susceptible", sf::Color(50, 50, 50), legendY + 110);
        drawLegendItem("Religious Zone", sf::Color(168, 85, 247), legendY + 140,
                       false);
        drawLegendItem("Workplace Zone", sf::Color(245, 158, 11), legendY + 160,
                       false);
      }

      sf::Text help(font,
                    "Space: Play/Pause | R: Reset\nArrows: Seek | "
                    "Tabs: Toggle Claim\nClick tabs for Truth/Misinfo focus",
                    12);
      help.setPosition({(float)WINDOW_SIZE + 20, 740});
      help.setFillColor(sf::Color(100, 100, 100));
      window.draw(help);
    }

    window.display();
  }
  return 0;
}
