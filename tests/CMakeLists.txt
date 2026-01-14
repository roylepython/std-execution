cmake_minimum_required(VERSION 3.25)
project(DualStackNet26Tests)

# Find the main library
find_package(DualStackNet26 REQUIRED)

# Create test executable
add_executable(dualstack_tests
    test_main.cpp
    test_ip_address.cpp
    test_socket.cpp
    test_performance.cpp
)

# Link against the library and testing framework
target_link_libraries(dualstack_tests 
    DualStackNet26::dualstack_net26
)

# Add test to CTest
add_test(NAME DualStackNet26Tests COMMAND dualstack_tests)

# Enable testing
enable_testing()