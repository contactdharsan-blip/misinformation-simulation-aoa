# Misinformation Simulation AoA

An Agent-Based Model (ABM) designed to simulate the spread of misinformation and truth within a synthetic model of Phoenix, Arizona.

## Core Model: SEDPNR
The simulation utilizes the SEDPNR epidemiological model to track agent states regarding a claim:
- **S (Susceptible)**: Has not yet encountered the claim.
- **E (Exposed)**: Has been exposed via social connections.
- **D (Doubtful)**: Evaluating the claim; requires social reinforcement to progress.
- **P (Propagating)**: Has adopted and is actively spreading the claim.
- **N (Not-Spreading)**: Has adopted but is not actively sharing.
- **R (Recovered)**: Has rejected the claim or ceased spreading it.

## Phoenix Demographics (2024 Estimates)
The population generation is calibrated to current Phoenix, AZ demographic data:
- **Age Distribution**: Median age ~35, with a significant 20-39 young adult segment.
- **Ethnicity**: Weighted towards Hispanic (~42%) and White (~41%) populations.
- **Religion**: Diverse landscape with significant Catholic, Evangelical, and Unaffiliated segments.

## Key Mechanics
1. **Identity-Based Interaction**: Exposure is weighted by demographic similarity. In the current configuration, **every agent** is assigned to a school-district hub, which acts as a primary center for interaction regardless of age. Agents are also more likely to interact with those in the same age group, ethnicity, or workplace.
2. **Social Reinforcement**: Transition from Exposed to Doubtful, and Doubtful to Adopted, is reinforced by the number of neighbors who have already adopted the claim.
3. **Misinformation Advantage**: Misinformation claims feature a `misinfo_multiplier` (default 6.0) representing higher viral potential and a lower `threshold` for adoption (0.25 vs 0.8 for truth).

## Key Parameters (`parameters.cfg`)
Current simulation configuration:
- **Population**: 10,000 agents in a single district (Phoenix Central).
- **Base Interaction Prob**: 0.05 (modulated by shared locations).
- **Adoption Probability (D->P)**: 0.20 (High viral potential).
- **Recovery Probability (P->R)**: 0.02 (Longer infectious period).
- **Homophily Strength**: 1.0 (Linear demographic weighting).

## Installation & Usage

### Prerequisites
To build and run this simulation, you will need:
- **C++ Compiler**: Support for C++17 or higher (GCC 9+ or Clang 10+).
- **SFML Library**: Used for the visualizer.
- **Python 3**: Used for demographic analysis scripts.

#### Installing Dependencies (macOS)
Using [Homebrew](https://brew.sh/):
```bash
brew install sfml python
```

#### Installing Dependencies (Windows)
The most compatible way to run this on Windows is using **MSYS2** to provide a Unix-like environment:
1. Download and install [MSYS2](https://www.msys2.org/).
2. Open the **UCRT64** terminal and install the toolchain:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-sfml mingw-w64-ucrt-x86_64-python
   ```
3. (Optional) Alternatively, use **WSL (Windows Subsystem for Linux)** and follow Linux instructions (`sudo apt install libsfml-dev`).

> **Note**: On Windows with MSYS2, you may need to use `mingw32-make` instead of `make`.

### Building
The project uses a standard Makefile. To build both the simulation and the visualizer:
```bash
make clean && make
```

### Running
To run the core simulation:
```bash
./simulation
```
Results are saved to `output/simulation_results.csv` and `output/spatial_data.csv`.

### Analysis
A Python script is provided to analyze demographic clusters:
```bash
python3 analyze.py
```
