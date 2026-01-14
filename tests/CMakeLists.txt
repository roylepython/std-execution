cmake_minimum_required(VERSION 3.25)
project(DualStackNet26Tests)

# Create test executable
add_executable(dualstack_tests
    test_main.cpp
)

# Link against the library and testing framework
target_link_libraries(dualstack_tests 
    dualstack_net26
)

# Add test to CTest
add_test(NAME DualStackNet26Tests COMMAND dualstack_tests)

# Enable testing
enable_testing()