@echo off
REM DualStackNet26 Build and Test Script for Windows
REM This script demonstrates the reproducible and empirically provable nature of the library

echo ========================================
echo DualStackNet26 Build and Test Suite
echo ========================================
echo.

REM Create build directory
echo Creating build directory...
mkdir build 2>nul
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo ❌ CMake configuration failed!
    cd ..
    exit /b 1
)

REM Build the library
echo Building the library...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Build failed!
    cd ..
    exit /b 1
)

REM Build tests
echo Building tests...
cmake --build . --target dualstack_tests

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Test build failed!
    cd ..
    exit /b 1
)

REM Run tests
echo Running tests...
dualstack_tests.exe

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Tests failed!
    cd ..
    exit /b 1
)

echo.
echo ✅ All builds and tests completed successfully!
echo 🎉 DualStackNet26 is reproducible and empirically provable!

REM Show build artifacts
echo.
echo Build artifacts:
dir lib\
dir bin\

cd ..