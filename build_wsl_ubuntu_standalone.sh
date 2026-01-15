#!/bin/bash
# Build script for std_execution (Amphisbaena) library on WSL/Ubuntu
# Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025

set -e  # Exit on error

echo "🐍 Building Amphisbaena (std_execution) Library - Standalone"
echo "============================================================"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check for required tools
echo -e "${YELLOW}Checking build tools...${NC}"
command -v cmake >/dev/null 2>&1 || { echo -e "${RED}cmake is required but not installed. Aborting.${NC}" >&2; exit 1; }
command -v g++ >/dev/null 2>&1 || { echo -e "${RED}g++ is required but not installed. Aborting.${NC}" >&2; exit 1; }

# Check GCC version (need 14.2.0+)
GCC_VERSION=$(g++ -dumpversion | cut -d. -f1)
if [ "$GCC_VERSION" -lt 14 ]; then
    echo -e "${RED}GCC 14.2.0+ required. Found: $(g++ --version | head -n1)${NC}"
    exit 1
fi

# Create build directory
BUILD_DIR="build-standalone"
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=26 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_FLAGS="-std=c++2c -Wall -Wextra -Wpedantic -O3" \
    -DBUILD_SHARED_LIBS=ON

# Build
echo -e "${YELLOW}Building library...${NC}"
make -j$(nproc)

# Run compilation test
echo -e "${YELLOW}Running compilation test...${NC}"
if ./compilation_test; then
    echo -e "${GREEN}✅ Compilation test passed!${NC}"
else
    echo -e "${RED}❌ Compilation test failed!${NC}"
    exit 1
fi

# Run unit tests if available
if [ -f "tests/dualstack_tests" ]; then
    echo -e "${YELLOW}Running unit tests...${NC}"
    if ./tests/dualstack_tests; then
        echo -e "${GREEN}✅ Unit tests passed!${NC}"
    else
        echo -e "${RED}❌ Unit tests failed!${NC}"
        exit 1
    fi
fi

# Show library info
echo -e "${GREEN}✅ Build successful!${NC}"
echo ""
echo "Library location: $(pwd)/libdualstack_net26.so"
if [ -f "libdualstack_net26.so" ]; then
    ls -lh libdualstack_net26.so
    file libdualstack_net26.so
    echo ""
    echo "To use this library:"
    echo "  export LD_LIBRARY_PATH=\$(pwd):\$LD_LIBRARY_PATH"
fi

cd ..

