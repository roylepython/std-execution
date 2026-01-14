#!/bin/bash

# DualStackNet26 Build and Test Script
# This script demonstrates the reproducible and empirically provable nature of the library

echo "========================================"
echo "DualStackNet26 Build and Test Suite"
echo "========================================"
echo

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed!"
    exit 1
fi

# Build the library
echo "Building the library..."
cmake --build . --config Release

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

# Build tests
echo "Building tests..."
cmake --build . --target dualstack_tests

if [ $? -ne 0 ]; then
    echo "❌ Test build failed!"
    exit 1
fi

# Run tests
echo "Running tests..."
./dualstack_tests

if [ $? -ne 0 ]; then
    echo "❌ Tests failed!"
    exit 1
fi

echo
echo "✅ All builds and tests completed successfully!"
echo "🎉 DualStackNet26 is reproducible and empirically provable!"

# Show build artifacts
echo
echo "Build artifacts:"
ls -la lib/
ls -la bin/

cd ..