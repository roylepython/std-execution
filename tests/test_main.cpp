#include "test_framework.h"
#include "test_ip_address.h"
#include "test_socket.h"
#include "test_performance.h"

using namespace dualstack::test;

auto main() -> int {
    std::cout << "DualStackNet26 Test Suite" << std::endl;
    std::cout << "=========================" << std::endl;
    
    bool all_passed = true;
    
    // Run IP Address tests
    all_passed &= run_ip_address_tests();
    
    // Run Socket tests
    all_passed &= run_socket_tests();
    
    // Run Performance tests
    all_passed &= run_performance_tests();
    
    std::cout << std::endl;
    if (all_passed) {
        std::cout << "🎉 All test suites passed!" << std::endl;
        return 0;
    } else {
        std::cout << "❌ Some test suites failed!" << std::endl;
        return 1;
    }
}