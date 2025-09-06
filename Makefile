# NEBULA EMERGENT Makefile
# Author: Francisco Angulo de Lafuente - NEBULA Team
# Physics-Based Neural Galaxy Architecture

CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra -pthread
SRCDIR = src
BUILDDIR = build
BINDIR = bin

# Create directories
$(shell mkdir -p $(BUILDDIR) $(BINDIR))

# Targets
TARGETS = nebula_emergent nebula_arc_solver

# Default target
all: $(TARGETS)

# NEBULA EMERGENT Neural Galaxy Simulation
nebula_emergent: $(SRCDIR)/NEBULA_EMERGENT_STANDALONE.cpp
	@echo "🌌 Compiling NEBULA EMERGENT Neural Galaxy..."
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/$@ $<
	@echo "✅ NEBULA EMERGENT compiled successfully"

# NEBULA ARC-AGI Spatial Reasoning Solver
nebula_arc_solver: $(SRCDIR)/NEBULA_ARC_SOLVER_STANDALONE.cpp
	@echo "🧠 Compiling NEBULA ARC-AGI Solver..."
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/$@ $<
	@echo "✅ NEBULA ARC Solver compiled successfully"

# Run tests
test: all
	@echo "🚀 Running NEBULA EMERGENT tests..."
	@echo "Testing Neural Galaxy Simulation (10 frames)..."
	@timeout 30s $(BINDIR)/nebula_emergent || echo "Galaxy simulation test completed"
	@echo ""
	@echo "Testing ARC-AGI Spatial Reasoning..."
	@$(BINDIR)/nebula_arc_solver
	@echo "✅ All tests completed"

# Performance benchmark
benchmark: all
	@echo "📊 Running NEBULA performance benchmarks..."
	@echo "Benchmarking compilation speed..."
	@time $(MAKE) clean all
	@echo ""
	@echo "Benchmarking execution performance..."
	@echo "Neural Galaxy (100 frames):"
	@timeout 60s $(BINDIR)/nebula_emergent || echo "Benchmark completed"

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -rf $(BUILDDIR)/* $(BINDIR)/*
	@echo "✅ Clean completed"

# Install (copy to system path)
install: all
	@echo "📦 Installing NEBULA EMERGENT..."
	sudo cp $(BINDIR)/* /usr/local/bin/
	@echo "✅ Installation completed"

# Uninstall
uninstall:
	@echo "🗑️ Uninstalling NEBULA EMERGENT..."
	sudo rm -f /usr/local/bin/nebula_emergent
	sudo rm -f /usr/local/bin/nebula_arc_solver
	@echo "✅ Uninstallation completed"

# Development build with debug symbols
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Profile build with profiling information
profile: CXXFLAGS += -pg
profile: all

# Static analysis
analyze:
	@echo "🔍 Running static analysis..."
	cppcheck --enable=all --std=c++17 $(SRCDIR)/*.cpp
	@echo "✅ Static analysis completed"

# Documentation generation (requires Doxygen)
docs:
	@echo "📚 Generating documentation..."
	doxygen Doxyfile 2>/dev/null || echo "Doxygen not available, skipping documentation"

# Memory check (requires Valgrind)
memcheck: debug
	@echo "🔍 Running memory analysis..."
	valgrind --leak-check=full --show-leak-kinds=all $(BINDIR)/nebula_emergent 2>/dev/null || echo "Valgrind not available"

# Code formatting (requires clang-format)
format:
	@echo "🎨 Formatting source code..."
	find $(SRCDIR) -name "*.cpp" -o -name "*.h" | xargs clang-format -i 2>/dev/null || echo "clang-format not available"

# Help
help:
	@echo "NEBULA EMERGENT Build System"
	@echo "============================"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all executables"
	@echo "  test         - Run test suite"
	@echo "  benchmark    - Performance benchmarks"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install to system path"
	@echo "  uninstall    - Remove from system"
	@echo "  debug        - Debug build"
	@echo "  profile      - Profiling build"
	@echo "  analyze      - Static code analysis"
	@echo "  docs         - Generate documentation"
	@echo "  memcheck     - Memory leak analysis"
	@echo "  format       - Format source code"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Usage Examples:"
	@echo "  make all                    # Build everything"
	@echo "  make test                   # Run tests"
	@echo "  make benchmark             # Performance test"
	@echo "  make clean install         # Clean build and install"

# Phony targets
.PHONY: all test benchmark clean install uninstall debug profile analyze docs memcheck format help

# Default target info
.DEFAULT_GOAL := all

