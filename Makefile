# Detector for OS
UNAME_S := $(shell uname -s)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Default paths (standard for Linux/MSYS2)
INCLUDES = -Iinclude
LDFLAGS = 

# macOS specific overrides (Homebrew)
ifeq ($(UNAME_S),Darwin)
	INCLUDES += -I/opt/homebrew/include
	LDFLAGS += -L/opt/homebrew/lib
endif
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = .

SIM_TARGET = $(BIN_DIR)/simulation
VIS_TARGET = $(BIN_DIR)/visualizer

SIM_SOURCES = src/main.cpp
VIS_SOURCES = src/visualizer.cpp

SIM_OBJECTS = $(OBJ_DIR)/main.o
VIS_OBJECTS = $(OBJ_DIR)/visualizer.o

.PHONY: all clean directories build-sim build-vis

all: directories build-sim build-vis

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p output

build-sim: $(SIM_TARGET)
build-vis: $(VIS_TARGET)

$(SIM_TARGET): $(SIM_OBJECTS)
	$(CXX) $(CXXFLAGS) $(SIM_OBJECTS) -o $(SIM_TARGET)

$(VIS_TARGET): $(VIS_OBJECTS)
	$(CXX) $(CXXFLAGS) $(VIS_OBJECTS) $(LDFLAGS) $(SFML_LIBS) -o $(VIS_TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(SIM_TARGET) $(VIS_TARGET) output/*.csv

run: simulation
	./$(SIM_TARGET)

view: visualizer
	./$(VIS_TARGET)
