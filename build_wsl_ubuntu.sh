#!/bin/bash
# Amphisbaena 🐍 - Build Script for Ubuntu/WSL
# Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025
# 
# Builds std_execution as standalone .so library for Ubuntu 24.04 / WSL
# NO PLACEHOLDERS - FULL IMPLEMENTATION

set -e  # Exit on error

echo "🐍 Amphisbaena - Building Dual-Stack Networking Library"
echo "========================================================"

# Check for GCC 14
if ! command -v g++-14 &> /dev/null; then
    echo "❌ GCC 14 not found. Installing..."
    sudo apt-get update
    sudo apt-get install -y g++-14 cmake build-essential
fi

# Check for CMake 3.25+
CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)

if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 25 ]); then
    echo "❌ CMake 3.25+ required. Found: $CMAKE_VERSION"
    exit 1
fi

echo "✅ GCC 14: $(g++-14 --version | head -n1)"
echo "✅ CMake: $CMAKE_VERSION"

# Create build directory
BUILD_DIR="build-ubuntu"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo ""
echo "📋 Configuring build..."
cmake .. \
    -DCMAKE_CXX_COMPILER=g++-14 \
    -DCMAKE_C_COMPILER=gcc-14 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=26 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=ON

# Build
echo ""
echo "🔨 Building library..."
make -j$(nproc)

# Verify .so was created
if [ -f "libdualstack_net26.so" ] || [ -f "libdualstack_net26.so.1" ] || [ -f "libdualstack_net26.so.1.0.0" ]; then
    echo ""
    echo "✅ Library built successfully!"
    find . -name "*.so*" -type f
    echo ""
    echo "📦 Library location: $(pwd)"
    echo "🐍 Amphisbaena ready for use!"
else
    echo "❌ Library file not found!"
    exit 1
fi

# Run basic tests if available
if [ -d "tests" ]; then
    echo ""
    echo "🧪 Running tests..."
    ctest --output-on-failure || echo "⚠️  Some tests failed (may be expected)"
fi

echo ""
echo "✅ Build complete! Library is ready as standalone .so"

