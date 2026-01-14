#!/bin/bash

# Amphisbaena Build Script for WSL (Ubuntu 24.04)
# Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved

set -e  # Exit on any error

echo "🐍 Amphisbaena Build System Initializing..."
echo "=========================================="

# Check if we're on Ubuntu 24.04
if ! grep -q "Ubuntu 24.04" /etc/os-release 2>/dev/null; then
    echo "⚠️  Warning: Not running on Ubuntu 24.04. Proceeding anyway..."
fi

# Check for required tools
echo "🔧 Checking prerequisites..."

REQUIRED_TOOLS=("cmake" "g++" "git" "make")
for tool in "${REQUIRED_TOOLS[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
        echo "❌ $tool is not installed. Please install it first."
        exit 1
    fi
done

echo "✅ All required tools found"

# Check for PsiForceDB integration
PSIFORCE_PATH="../Projects/LamiaFabrica/Back-Office/PsiForceDB_1.0.0"
if [ ! -d "$PSIFORCE_PATH" ]; then
    echo "⚠️  Warning: PsiForceDB not found at $PSIFORCE_PATH"
    echo "💡 Please ensure PsiForceDB is properly integrated"
fi

# Create build directory
echo "🏗️  Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "⚙️  Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo "🔨 Building Amphisbaena..."
make -j$(nproc)

# Run tests
echo "🧪 Running tests..."
make test

# Show build results
echo ""
echo "🎉 Amphisbaena Build Complete!"
echo "=============================="
echo "📦 Built artifacts:"
echo "   - libdualstack_net26.a (static library)"
echo "   - libdualstack_net26.so (shared library)"
echo "   - example_server (demo application)"
echo "   - concurrent_dualstack_server (concurrent demo)"
echo "   - secure_dualstack_server (secure demo)"
echo "   - compilation_test (verification)"
echo ""
echo "🚀 To run the secure server demo:"
echo "   ./examples/secure_dualstack_server"
echo ""
echo "📖 For documentation, see README_AMPHISBAENA.md"
echo ""
echo "🐍 Amphisbaena - The Dual-Headed Networking Library"
echo "   Named after the legendary dual-headed serpent"
echo "   Bringing quantum-resistant security to the modern age"