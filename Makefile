# Makefile for Network Simulator
# Compiler and flags
CXX             := g++
CXXFLAGS        := -std=c++17 -Wall -Wextra -I.
LDFLAGS         :=
# Build directories
BUILD_DIR       := build
OBJ_DIR         := $(BUILD_DIR)/obj
DEP_DIR         := $(BUILD_DIR)/deps
# Output executables
TARGET          := $(BUILD_DIR)/simulator
BACKEND_TARGET  := $(BUILD_DIR)/backend-runner

# Data output directory
DATA_DIR        := data_output
# Source files organized by directory
ENGINE_SRC      := src/main.cpp \
                   engine/config.cpp \
                   engine/data_collector.cpp \
                   engine/event_queue.cpp \
                   engine/integration.cpp \
                   engine/node.cpp \
                   engine/packet.cpp \
                   engine/packet_flow.cpp \
                   engine/pipeline.cpp \
                   engine/simulator_clock.cpp \
                   engine/simulator.cpp \
                   engine/statistics.cpp

NETWORK_SRC     := network/congestion.cpp \
                   network/debug.cpp \
                   network/failure.cpp \
                   network/logger.cpp \
                   network/metrics.cpp \
                   network/network_layer.cpp \
                   network/queue.cpp \
                   network/tcp.cpp \
                   network/traffic_generator.cpp \
                   network/udp.cpp

GRAPHS_SRC      := graphs/graph_analysis.cpp \
                   graphs/graph_generator.cpp \
                   graphs/graphs.cpp

ALGORITHMS_SRC  := algorithms/shortest_path.cpp \
                   algorithms/mst.cpp \
                   algorithms/union_find.cpp \
                   algorithms/routing_table.cpp \
                   algorithms/metrics.cpp

PERFORMANCE_SRC := performance/profiler.cpp \
                   performance/stress_test.cpp

# Backend runner sources (web API binary)
# Note: Uses simulation_runner instead of main, but all the same dependencies
BACKEND_RUNNER_MAIN := backend/simulation_runner.cpp
BACKEND_ENGINE_SRC  := engine/config.cpp \
                       engine/data_collector.cpp \
                       engine/event_queue.cpp \
                       engine/integration.cpp \
                       engine/node.cpp \
                       engine/packet.cpp \
                       engine/packet_flow.cpp \
                       engine/pipeline.cpp \
                       engine/simulator_clock.cpp \
                       engine/simulator.cpp \
                       engine/statistics.cpp

BACKEND_SRC         := $(BACKEND_RUNNER_MAIN) \
                       $(BACKEND_ENGINE_SRC) \
                       $(NETWORK_SRC) \
                       $(GRAPHS_SRC) \
                       $(ALGORITHMS_SRC) \
                       performance/profiler.cpp

# Combine all sources
SOURCES         := $(ENGINE_SRC) $(NETWORK_SRC) $(GRAPHS_SRC) $(ALGORITHMS_SRC) $(PERFORMANCE_SRC)
OBJECTS         := $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)
DEPS            := $(SOURCES:%.cpp=$(DEP_DIR)/%.d)

# Default mode is release
MODE            ?= release

# Mode-specific flags
ifeq ($(MODE),debug)
    CXXFLAGS    += -g -O0 -DDEBUG
    TARGET      := $(BUILD_DIR)/simulator_debug
else ifeq ($(MODE),release)
    CXXFLAGS    += -O2 -DNDEBUG
else
    $(error Invalid MODE. Use 'make MODE=debug' or 'make MODE=release')
endif

# Targets

.PHONY: all debug release clean rebuild help data-dir backend-runner

# Default target
all: $(TARGET) $(BACKEND_TARGET)

# Debug build
debug:
	@$(MAKE) MODE=debug

# Release build
release:
	@$(MAKE) MODE=release

# Main executable
$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	@echo "[LINK] Linking $@..."
	@$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "[SUCCESS] Built $@"

# Backend API runner (for web server)
$(BACKEND_TARGET): $(BACKEND_SRC) | $(BUILD_DIR)
	@echo "[LINK] Linking backend runner $@..."
	@$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	@echo "[SUCCESS] Built backend runner $@"

# Object files
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR) $(DEP_DIR)
	@echo "[CXX] Compiling $<..."
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEP_DIR)/$*)
	@$(CXX) $(CXXFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

# Create directories
$(BUILD_DIR) $(OBJ_DIR) $(DEP_DIR):
	@mkdir -p $@

# Include dependency files
-include $(DEPS)

# Clean build artifacts
clean:
	@echo "[CLEAN] Removing build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "[CLEAN] Done"

# Rebuild
rebuild: clean all

# Create data output directory
data-dir:
	@echo "[DATA] Creating data output directory..."
	@mkdir -p $(DATA_DIR)
	@echo "[DATA] Data directory created: $(DATA_DIR)"

# Help
help:
	@echo "Network Simulator - Makefile targets:"
	@echo ""
	@echo "  make              Build both simulator and backend-runner (default: release)"
	@echo "  make debug        Build with debug symbols"
	@echo "  make release      Build optimized release"
	@echo "  make clean        Remove all build files"
	@echo "  make rebuild      Clean and rebuild"
	@echo "  make data-dir     Create data output directory"
	@echo "  make run          Build and run main simulator"
	@echo "  make run-debug    Build debug version and run"
	@echo "  make help         Show this message"
	@echo ""
	@echo "Binaries:"
	@echo "  build/simulator       Main simulator (CLI, single run, stats collection)"
	@echo "  build/backend-runner  Backend runner (web API, JSON output)"
	@echo ""
	@echo "Environment variables:"
	@echo "  MODE=debug        Compile with debug symbols and no optimization"
	@echo "  MODE=release      Compile with optimization (default)"
	@echo ""
	@echo "Example:"
	@echo "  make MODE=debug   # Build debug versions of both binaries"
	@echo "  make release      # Build optimized release"

# Run targets
run: release
	@echo "[RUN] Starting simulator..."
	@$(BUILD_DIR)/simulator

run-debug: debug
	@echo "[RUN] Starting simulator (debug)..."
	@$(BUILD_DIR)/simulator_debug

.SILENT: clean help
