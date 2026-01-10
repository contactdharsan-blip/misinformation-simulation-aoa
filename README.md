# Misinformation Simulation AoA

An advanced Agent-Based Model (ABM) for simulating the spread of misinformation using the **SEDPNR** (Susceptible, Exposed, Doubtful, Propagating, Not-Spreading, Recovered) model. This simulation models a population across multiple towns with specific demographic attributes and location-based social networks (Schools, Religious Establishments).

## Features

*   **SEDPNR Model**: A sophisticated state-transition model for belief dynamics.
*   **Demographic Agents**: Agents have age, education, and ethnicity attributes that influence their credibility and interaction probabilities.
*   **Location-Based Social Network**: Agents interact based on shared locations:
    *   **Schools**: Assigned based on age.
    *   **Religious Establishments**: Assigned probabilistically.
    *   **Towns**: General spatial proximity.
*   **Configurable Parameters**: Almost every aspect of the simulation can be tuned via `parameters.cfg`.
*   **Dual-View Visualizer**: A C++ SFML-based visualizer to observe the spread in real-time.
    *   **Grid View**: High-level overview of all towns.
    *   **District View**: Detailed inspection of individual towns with independent location layouts.

## New in this Version

*   **Religious Participation**: Not all agents are religious. A new parameter `religious_participation_prob` (default 0.6) controls the percentage of the population assigned to religious establishments.
*   **Detailed District View**: Inspect specific towns to see exact school and church locations and agent clustering.

## Prerequisites

*   **C++ Compiler**: Must support C++17 (e.g., `g++`, `clang++`).
*   **SFML 3.0**: Required for the visualizer. Ensure it is installed and accessible (e.g., via Homebrew on macOS: `brew install sfml`).

## Installation & Build

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/contactdharsan-blip/misinformation-simulation-aoa.git
    cd misinformation-simulation-aoa
    ```

2.  **Build and Run Simulation**:
    This command compiles the core simulation, generates the agent population, runs the time steps, and outputs data.
    ```bash
    make run
    ```

3.  **Build Visualizer**:
    This compiles the graphical visualizer.
    ```bash
    make build-vis
    ```

## Usage

### 1. Configure
Edit `parameters.cfg` to tune the simulation. Key parameters include:
*   `population`: Total number of agents.
*   `religious_participation_prob`: Probability (0.0 - 1.0) of an agent joining a religious community.
*   `misinfo_multiplier`: How much faster misinformation spreads compared to truth.
*   `truth_threshold` / `misinfo_threshold`: Credibility thresholds for adoption.

### 2. Run Simulation
Execute the simulation to generate the dataset (`output/spatial_data.csv`).
```bash
./simulation
```

### 3. Run Visualizer
Launch the visualizer to replay the simulation data.
```bash
./visualizer
```

### Visualizer Controls

| Key | Action |
| :--- | :--- |
| **Space** | Play / Pause playback |
| **R** | Reset time to 0 |
| **Left / Right Arrow** | Seek backward / forward |
| **Up / Down Arrow** | Increase / Decrease playback speed |
| **0 - 9** | Switch active Claim view (0 = Truth, 1-9 = Misinfo) |
| **Tab** | Toggle between **GRID VIEW** and **DISTRICT VIEW** |
| **[** | Previous District (cycle through towns) |
| **]** | Next District (cycle through towns) |

## Output

*   `output/simulation_results.csv`: Summary statistics for runs.
*   `output/spatial_data.csv`: Detailed time-series data for agent states and positions, used by the visualizer.

