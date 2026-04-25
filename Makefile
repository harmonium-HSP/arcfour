#!/usr/bin/make -f
# Makefile for arcfour library
#
# Usage:
#   make build        - Build the project
#   make test         - Run tests
#   make clean        - Clean build directory
#   make coverage     - Generate coverage report
#   make install      - Install the library
#   make uninstall    - Uninstall the library
#   make docs         - Build documentation
#   make format       - Format source code
#   make check        - Run static analysis

# Configuration
BUILD_DIR ?= build
BUILD_TYPE ?= Release
CMAKE_FLAGS ?=

# Default target
all: build

# Build the project
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_FLAGS)
	@cd $(BUILD_DIR) && make -j$(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Run tests
test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

# Clean build directory
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf CMakeCache.txt CMakeFiles

# Generate coverage report
coverage:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
	@cd $(BUILD_DIR) && make -j$(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@cd $(BUILD_DIR) && ctest
	@cd $(BUILD_DIR) && make coverage

# Install the library
install: build
	@cd $(BUILD_DIR) && make install

# Uninstall the library
uninstall:
	@cd $(BUILD_DIR) && make uninstall

# Build documentation
docs:
	@cd docs && make html

# Format source code
format:
	@find src include test -name "*.c" -o -name "*.h" | xargs clang-format -i

# Run static analysis
check:
	@cd $(BUILD_DIR) && make check

# Build debug version
debug:
	@$(MAKE) build BUILD_TYPE=Debug

# Build release version
release:
	@$(MAKE) build BUILD_TYPE=Release

# Build with ASAN
asan:
	@$(MAKE) build CMAKE_FLAGS="-DENABLE_ASAN=ON"

# Build with UBSAN
ubsan:
	@$(MAKE) build CMAKE_FLAGS="-DENABLE_UBSAN=ON"

# Print help
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  build          Build the project (default)"
	@echo "  test           Run tests"
	@echo "  clean          Clean build directory"
	@echo "  coverage       Generate coverage report"
	@echo "  install        Install the library"
	@echo "  uninstall      Uninstall the library"
	@echo "  docs           Build documentation"
	@echo "  format         Format source code"
	@echo "  check          Run static analysis"
	@echo "  debug          Build debug version"
	@echo "  release        Build release version"
	@echo "  asan           Build with AddressSanitizer"
	@echo "  ubsan          Build with UndefinedBehaviorSanitizer"
	@echo "  help           Print this help message"

.PHONY: all build test clean coverage install uninstall docs format check debug release asan ubsan help