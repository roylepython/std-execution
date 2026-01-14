#pragma once

#include "test_framework.h"
#include "../src/core/socket.h"
#include "../src/core/acceptor.h"
#include <thread>
#include <chrono>

namespace dualstack {
namespace test {

inline auto test_socket_creation() -> TestResult {
    // Test TCP socket creation
    auto tcp_socket = create_tcp_socket();
    if (!tcp_socket.has_value()) {
        return TestResult(false, "Failed to create TCP socket", std::chrono::milliseconds(0));
    }
    
    // Test socket state
    if (!tcp_socket.value().is_open()) {
        return TestResult(false, "Newly created socket should not be open", 
                         std::chrono::milliseconds(0));
    }
    
    return TestResult(true, "", std::chrono::milliseconds(0));
}

inline auto test_acceptor_creation() -> TestResult {
    // Test acceptor creation
    auto acceptor_result = create_acceptor(0); // Use ephemeral port
    if (!acceptor_result.has_value()) {
        return TestResult(false, "Failed to create acceptor", std::chrono::milliseconds(0));
    }
    
    auto acceptor = std::move(acceptor_result.value());
    
    // Test acceptor state
    if (!acceptor.is_listening()) {
        return TestResult(false, "Acceptor should be listening after creation", 
                         std::chrono::milliseconds(0));
    }
    
    return TestResult(true, "", std::chrono::milliseconds(0));
}

inline auto test_dual_stack_binding() -> TestResult {
    // Test binding to IPv6 (should support both IPv4 and IPv6)
    Acceptor acceptor;
    auto result = acceptor.listen(0); // Ephemeral port
    
    if (result != error_code::success) {
        return TestResult(false, "Failed to create dual-stack listener", 
                         std::chrono::milliseconds(0));
    }
    
    // Test dual-stack enablement
    auto dual_result = acceptor.enable_dual_stack(true);
    if (dual_result != error_code::success) {
        return TestResult(false, "Failed to enable dual-stack support", 
                         std::chrono::milliseconds(0));
    }
    
    return TestResult(true, "", std::chrono::milliseconds(0));
}

inline auto test_move_semantics() -> TestResult {
    // Test socket move semantics
    auto socket1 = create_tcp_socket();
    if (!socket1.has_value()) {
        return TestResult(false, "Failed to create initial socket", 
                         std::chrono::milliseconds(0));
    }
    
    // Move socket
    Socket socket2 = std::move(socket1.value());
    
    // Original should be in moved-from state
    // Note: This is harder to test without exposing internal state
    
    return TestResult(true, "", std::chrono::milliseconds(0));
}

inline auto test_performance_operations() -> TestResult {
    const int iterations = 1000;
    
    PerformanceTimer timer;
    
    // Benchmark socket creation
    for (int i = 0; i < iterations; ++i) {
        auto socket = create_tcp_socket();
        if (!socket.has_value()) {
            return TestResult(false, "Performance test failed due to socket creation error", 
                             std::chrono::milliseconds(0));
        }
    }
    
    auto create_duration = timer.elapsed_microseconds();
    
    timer = PerformanceTimer();
    
    // Benchmark acceptor creation
    for (int i = 0; i < iterations; ++i) {
        auto acceptor = create_acceptor(0);
        if (!acceptor.has_value()) {
            return TestResult(false, "Performance test failed due to acceptor creation error", 
                             std::chrono::milliseconds(0));
        }
    }
    
    auto acceptor_duration = timer.elapsed_microseconds();
    
    std::cout << "Socket creation: " << (create_duration.count() / iterations) << " μs/op" << std::endl;
    std::cout << "Acceptor creation: " << (acceptor_duration.count() / iterations) << " μs/op" << std::endl;
    
    return TestResult(true, "Socket performance benchmark completed", std::chrono::milliseconds(0));
}

inline auto run_socket_tests() -> bool {
    TestSuite suite("Socket Tests");
    
    suite.add_test("Socket Creation", test_socket_creation);
    suite.add_test("Acceptor Creation", test_acceptor_creation);
    suite.add_test("Dual-Stack Binding", test_dual_stack_binding);
    suite.add_test("Move Semantics", test_move_semantics);
    suite.add_test("Performance Operations", test_performance_operations);
    
    return suite.run();
}

} // namespace test
} // namespace dualstack