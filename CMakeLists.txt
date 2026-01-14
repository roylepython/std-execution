cmake_minimum_required(VERSION 3.25)
project(DualStackNet26 VERSION 1.0.0 LANGUAGES CXX)

# C++26 standard support
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Enable experimental C++26 features
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2c")
    
    # Enable specific features
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts-ts")
    
    # Warning flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    
    # Optimization flags
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
    
    # Warning flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific libraries
    set(PLATFORM_LIBS ws2_32)
else()
    # Unix-specific libraries
    set(PLATFORM_LIBS pthread)
endif()

# Source files
set(SOURCES
    src/core/ip_address.cpp
    src/core/socket.cpp
    src/core/acceptor.cpp
    src/async/execution.cpp
    src/security/security.cpp
    src/performance/optimization.cpp
)

# Header files for installation
set(HEADERS
    src/core/ip_address.h
    src/core/socket.h
    src/core/acceptor.h
    src/async/execution.h
    src/reflect/reflection.h
    src/security/security.h
    src/performance/optimization.h
)

# Create the library
add_library(dualstack_net26 ${SOURCES})

# Include directories
target_include_directories(dualstack_net26 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Link platform-specific libraries
target_link_libraries(dualstack_net26 PRIVATE ${PLATFORM_LIBS})

# Compile definitions
target_compile_definitions(dualstack_net26 PRIVATE
    $<$<BOOL:${WIN32}>:WIN32>
)

# Installation rules
include(GNUInstallDirs)
install(TARGETS dualstack_net26
    EXPORT DualStackNet26Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dualstack_net26
    FILES_MATCHING PATTERN "*.h"
)

# Export targets
install(EXPORT DualStackNet26Targets
    FILE DualStackNet26Targets.cmake
    NAMESPACE DualStackNet26::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DualStackNet26
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/DualStackNet26Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DualStackNet26Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DualStackNet26
)

# Install config file
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DualStackNet26Config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DualStackNet26
)

# Example application
add_executable(example_server examples/server.cpp)
target_link_libraries(example_server dualstack_net26)

# Test compilation executable
add_executable(compilation_test src/main.cpp)
target_link_libraries(compilation_test dualstack_net26)

# Tests
enable_testing()
add_subdirectory(tests)

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    add_subdirectory(docs)
endif()