@echo off
REM DualStackNet26 - Amphisbaena 🐍
REM Windows Build Script
REM Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved

echo ========================================
echo DualStackNet26 Windows Build Script
echo ========================================

REM Check for Visual Studio compiler
where cl >nul 2>&1
if %errorlevel% == 0 (
    echo Found Visual Studio compiler
    goto build_vs
)

REM Check for MinGW compiler
where g++ >nul 2>&1
if %errorlevel% == 0 (
    echo Found MinGW compiler
    goto build_mingw
)

REM Check for Clang compiler
where clang++ >nul 2>&1
if %errorlevel% == 0 (
    echo Found Clang compiler
    goto build_clang
)

echo No compatible compiler found!
echo Please install one of:
echo - Visual Studio Build Tools
echo - MinGW-w64
echo - LLVM/Clang
exit /b 1

:build_vs
echo Building with Visual Studio compiler...
cl /std:c++latest /EHsc /W4 /O2 ^
   src/core/ip_address.cpp ^
   src/core/socket.cpp ^
   src/core/acceptor.cpp ^
   src/async/execution.cpp ^
   src/performance/optimization.cpp ^
   src/security/security.cpp ^
   src/security/tls_protocol.cpp ^
   src/main.cpp ^
   /link ws2_32.lib advapi32.lib /OUT:dualstack_net26.exe

if %errorlevel% == 0 (
    echo Build successful: dualstack_net26.exe
) else (
    echo Build failed!
    exit /b 1
)
goto end

:build_mingw
echo Building with MinGW compiler...
g++ -std=c++26 -Wall -Wextra -O3 ^
    -Isrc ^
    src/core/ip_address.cpp ^
    src/core/socket.cpp ^
    src/core/acceptor.cpp ^
    src/async/execution.cpp ^
    src/performance/optimization.cpp ^
    src/security/security.cpp ^
    src/security/tls_protocol.cpp ^
    src/main.cpp ^
    -lws2_32 -static-libgcc -static-libstdc++ ^
    -o dualstack_net26.exe

if %errorlevel% == 0 (
    echo Build successful: dualstack_net26.exe
) else (
    echo Build failed!
    exit /b 1
)
goto end

:build_clang
echo Building with Clang compiler...
clang++ -std=c++26 -Wall -Wextra -O3 ^
    -Isrc ^
    src/core/ip_address.cpp ^
    src/core/socket.cpp ^
    src/core/acceptor.cpp ^
    src/async/execution.cpp ^
    src/performance/optimization.cpp ^
    src/security/security.cpp ^
    src/security/tls_protocol.cpp ^
    src/main.cpp ^
    -lws2_32 ^
    -o dualstack_net26.exe

if %errorlevel% == 0 (
    echo Build successful: dualstack_net26.exe
) else (
    echo Build failed!
    exit /b 1
)
goto end

:end
echo ========================================
echo Build process completed!
echo ========================================