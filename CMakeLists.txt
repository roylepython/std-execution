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
    
    # Enable specific features (concepts are built-in to C++20+, so this is deprecated)
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts-ts")  # Deprecated in GCC 15
    
    # Fix for GCC 14.2.0 format header bug with C++26
    # The format header has a syntax error at line 3750 when using C++26
    # Store the fix header path for later use at target level
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "14.0" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
        # Get absolute path to fix header for -include flag
        get_filename_component(FIX_HEADER_ABS "${CMAKE_CURRENT_SOURCE_DIR}/include/dualstack_net26/fix_format_header.h" ABSOLUTE)
        set(FIX_HEADER_PATH "${FIX_HEADER_ABS}")
        message(STATUS "🔧 GCC 14.2.0 detected - auto-including fix_format_header.h")
        message(STATUS "   ✅ Format header fix will be applied via -include flag")
    endif()
    
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

# ============================================================================
# SSL/TLS Library Support: OpenSSL (default) or LFSSL (opt-in)
# ============================================================================
# Default to OpenSSL for compatibility with rest of world
# Opt-in to LFSSL for our applications and community
option(USE_LFSSL "Use LFSSL instead of OpenSSL" OFF)

# Variables to store SSL library configuration (linked after target creation)
set(SSL_LIBS "")
set(SSL_INCLUDE_DIRS "")
set(SSL_DEFINITIONS "")

if(USE_LFSSL)
    # LFSSL integration - our internal cryptographic library
    message(STATUS "🔒 Using LFSSL for cryptographic operations")
    if(WIN32)
        set(LFSSL_DIR "C:/McMaker Projects/Projects/LFSSL - Lamia Fabrica SSL")
    else()
        set(LFSSL_DIR "/mnt/c/McMaker Projects/Projects/LFSSL - Lamia Fabrica SSL")
    endif()
    if(EXISTS "${LFSSL_DIR}/CMakeLists.txt")
        add_subdirectory(${LFSSL_DIR} lfssl_build)
        set(SSL_LIBS lfssl)
        set(SSL_INCLUDE_DIRS ${LFSSL_DIR}/include)
        set(SSL_DEFINITIONS USE_LFSSL)
    else()
        message(WARNING "LFSSL not found at ${LFSSL_DIR}, falling back to OpenSSL")
        set(USE_LFSSL OFF)
    endif()
endif()

if(NOT USE_LFSSL)
    # OpenSSL - default for rest of world compatibility
    message(STATUS "🔒 Using OpenSSL for cryptographic operations (default)")
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        set(SSL_LIBS OpenSSL::SSL OpenSSL::Crypto)
        set(SSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
        set(SSL_DEFINITIONS USE_OPENSSL)
        message(STATUS "✅ OpenSSL found: ${OPENSSL_VERSION}")
    else()
        message(WARNING "⚠️  OpenSSL not found - TLS/SSL features may be unavailable")
        message(STATUS "   Consider setting USE_LFSSL=ON to use LFSSL instead")
    endif()
endif()

# Source files
set(SOURCES
    src/core/ip_address.cpp
    src/core/socket.cpp
    src/core/acceptor.cpp
    src/async/execution.cpp
    src/security/security.cpp
    src/security/adr_rdr.cpp
    src/security/signature_visualizer.cpp
    src/performance/optimization.cpp
    src/network/async_connection_manager.cpp
    src/network/notifications.cpp
    src/network/network_config.cpp
    src/network/virtual_adapter.cpp
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
    src/network/async_connection_manager.h
    include/dualstack_net26/network/notifications.h
    include/dualstack_net26/network/virtual_adapter.h
    include/dualstack_net26/network/network_config.h
)

# Create the library as SHARED (DLL/SO) for public distribution
# Check if target already exists (when included as subdirectory multiple times)
if(NOT TARGET dualstack_net26)
    add_library(dualstack_net26 SHARED ${SOURCES})

    # Set library properties for public API
    set_target_properties(dualstack_net26 PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${HEADERS}"
        DEFINE_SYMBOL "AMPHISBAENA_BUILDING_LIBRARY"
    )

    # Include directories - public API in include/, implementation in src/
    target_include_directories(dualstack_net26 PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    )

    # Link platform-specific libraries
    target_link_libraries(dualstack_net26 PRIVATE ${PLATFORM_LIBS})

    # Link SSL/TLS libraries (if configured)
    if(SSL_LIBS)
        target_link_libraries(dualstack_net26 PRIVATE ${SSL_LIBS})
    endif()
    if(SSL_INCLUDE_DIRS)
        target_include_directories(dualstack_net26 PUBLIC ${SSL_INCLUDE_DIRS})
    endif()
    if(SSL_DEFINITIONS)
        target_compile_definitions(dualstack_net26 PRIVATE ${SSL_DEFINITIONS})
    endif()

    # Compile definitions
    target_compile_definitions(dualstack_net26 PRIVATE
        $<$<BOOL:${WIN32}>:WIN32>
        AMPHISBAENA_BUILDING_LIBRARY
    )
    
    # Apply format header fix at target level (proper fix for GCC 14.2.0)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "14.0" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0" AND DEFINED FIX_HEADER_PATH)
        # Use -include to automatically include our fix header before every source file
        # This ensures the format header bug is fixed before any standard headers are included
        # CRITICAL: Undefine __cpp_lib_format first, then redefine to C++20 value
        # This prevents the compiler from setting it to C++26 value based on -std=c++2c
        # Also use -D to define macros BEFORE -include so they're available in the fix header
        # CRITICAL: Order matters! Undefine first, then define macros, then include fix header
        # The fix header will further ensure the macros are set correctly
        target_compile_options(dualstack_net26 PRIVATE 
            "-U__cpp_lib_format"  # Step 1: Undefine compiler's C++26 value
            "-D__cpp_lib_format=202110L"  # Step 2: Define C++20 format value
            "-D_GLIBCXX_USE_C99_STDIO=1"  # Step 3: Enable C99 features
            "-D_GLIBCXX_USE_C99_COMPLEX=1"
            "-D_GLIBCXX_HAVE_CXX26_FORMAT=0"  # Step 4: Disable C++26 format features
            "-D_GLIBCXX_NO_CXX26_FORMAT_FEATURES=1"
            "-D_GLIBCXX_USE_CXX20_FORMAT=1"  # Step 5: Force C++20 format path
            "-D_GLIBCXX_FORCE_CXX20_FORMAT=1"  # Step 6: Force C++20 format implementation
            "-D_GLIBCXX_NO_CXX26_ATTRIBUTES=1"  # Step 7: Disable C++26 attribute syntax
            "-include" "${FIX_HEADER_PATH}"  # Step 8: Include fix header (will verify/override macros)
        )
        # Also add as compile definitions for consistency
        target_compile_definitions(dualstack_net26 PRIVATE 
            "__cpp_lib_format=202110L"
            "_GLIBCXX_USE_C99_STDIO=1"
            "_GLIBCXX_USE_C99_COMPLEX=1"
            "_GLIBCXX_HAVE_CXX26_FORMAT=0"
            "_GLIBCXX_NO_CXX26_FORMAT_FEATURES=1"
            "_GLIBCXX_FORCE_CXX20_FORMAT=1"
            "_GLIBCXX_NO_CXX26_ATTRIBUTES=1"
        )
        message(STATUS "   Applied format header fix to dualstack_net26 target")
        message(STATUS "   Using -include flag with -D macros BEFORE -include for format fix")
    endif()
endif()

# Installation rules (only when building standalone, not as subdirectory)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include(GNUInstallDirs)
    install(TARGETS dualstack_net26
        EXPORT DualStackNet26Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dualstack_net26
    )

    # Install public headers from include/ directory
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )

    # Install implementation headers from src/ (for advanced users)
    install(DIRECTORY src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dualstack_net26
        FILES_MATCHING PATTERN "*.h"
        PATTERN "*.cpp" EXCLUDE
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
endif()

# Example applications (only build if not being included as subdirectory)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR OR BUILD_EXAMPLES)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/server.cpp")
        if(NOT TARGET example_server)
            add_executable(example_server examples/server.cpp)
            target_link_libraries(example_server dualstack_net26)
        endif()
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/dualstack_signature_demo.cpp")
        if(NOT TARGET dualstack_signature_demo)
            add_executable(dualstack_signature_demo examples/dualstack_signature_demo.cpp)
            target_link_libraries(dualstack_signature_demo dualstack_net26)
        endif()
    endif()
    
    # Test compilation executable
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
        if(NOT TARGET compilation_test)
            add_executable(compilation_test src/main.cpp)
            target_link_libraries(compilation_test dualstack_net26)
        endif()
    endif()
endif()

# Tests (only build if not being included as subdirectory)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR OR BUILD_TESTS)
    enable_testing()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        if(NOT TARGET dualstack_tests)
            add_subdirectory(tests)
        endif()
    endif()
endif()

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    add_subdirectory(docs)
endif()
# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    add_subdirectory(docs)
endif()