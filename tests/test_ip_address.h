#pragma once

#include "test_framework.h"
#include "../src/core/ip_address.h"

namespace dualstack {
namespace test {

inline auto test_ipv4_parsing() -> TestResult {
    // Test valid IPv4 addresses
    auto result1 = IPAddress::from_string("192.168.1.1");
    if (!result1.has_value()) {
        return TestResult(false, "Failed to parse valid IPv4 address 192.168.1.1", 
                         std::chrono::milliseconds(0));
    }
    
    auto result2 = IPAddress::from_string("0.0.0.0");
    if (!result2.has_value()) {
        return TestResult(false, "Failed to parse valid IPv4 address 0.0.0.0", 
                         std::chrono::milliseconds(0));
    }
    
    auto result3 = IPAddress::from_string("255.255.255.255");
    if (!result3.has_value()) {
        return TestResult(false, "Failed to parse valid IPv4 address 255.255.255.255", 
                         std::chrono::milliseconds(0));
    }
    
    // Test invalid IPv4 addresses
    auto invalid1 = IPAddress::from_string("999.999.999.999");
    if (invalid1.has_value()) {
        return TestResult(false, "Should fail to parse invalid IPv4 address 999.999.999.999", 
                         std::chrono::milliseconds(0));
    }
    
    auto invalid2 = IPAddress::from_string("192.168.1");
    if (invalid2.has_value()) {
        return TestResult(false, "Should fail to parse incomplete IPv4 address 192.168.1", 
                         std::chrono::milliseconds(0));
    }
    
    return TestResult(true, "", std::chrono::milliseconds(0));
}

inline auto test_ipv6_parsing() -> TestResult {
    // Test valid IPv6 addresses
    auto result1 = IPAddress::from_string("2001:db8::1");
    if (!result1.has_value()) {
        return TestResult(false, "Failed to parse valid IPv6 address 2001:db8::1", 
                         std::chrono::milliseconds(0));
    }
    
    auto result2 = IPAddress::from_string("::1");
    if (!result2.has_value()) {
        return TestResult(false, "Failed to parse valid IPv6 address ::1", 
                         std::chrono::milliseconds(0));
    }
    
    auto result3 = IPAddress::from_string("::");
    if (!result3.has_value()) {
        return TestResult(false, "Failed to parse valid IPv6 address ::", 
                         std::chrono::milliseconds(0));
    }
    
    // Test IPv6 round-trip
    auto addr = IPAddress::from_string("2001:db8::1");
    if (addr.has_value()) {
        auto str = addr.value().to_string();
        auto addr2 = IPAddress::from_string(str);
        if (!addr2.has_value() || addr2.value() != addr.value()) {
            return TestResult(false, "IPv6 round-trip conversion failed", 
                             std::chrono::milliseconds(0));
        }
    }
    
    return TestResult(true, "", std::chrono::milliseconds(0));
}

inline auto test_address_comparison() -> TestResult {
    // Test IPv4 comparison
    auto addr1 = IPAddress::from_string("192.168.1.1");
    auto addr2 = IPAddress::from_string("192.168.1.1");
    auto addr3 = IPAddress::from_string("192.168.1.2");
    
    if (!addr1.has_value() || !addr2.has_value() || !addr3.has_value()) {
        return TestResult(false, "Failed to create IPv4 addresses for comparison", 
                         std::chrono::milliseconds(0));
    }
    
    if (!(addr1.value() == addr2.value())) {
        return TestResult(false, "Equal IPv4 addresses should compare equal", 
                         std::chrono::milliseconds(0));
    }
    
    if (addr1.value() == addr3.value()) {
        return TestResult(false, "Different IPv4 addresses should not compare equal", 
                         std::chrono::milliseconds(0));
    }
    
    // Test IPv6 comparison
    auto addr4 = IPAddress::from_string("2001:db8::1");
    auto addr5 = IPAddress::from_string("2001:db8::1");
    auto addr6 = IPAddress::from_string("2001:db8::2");
    
    if (!addr4.has_value() || !addr5.has_value() || !addr6.has_value()) {
        return TestResult(false, "Failed to create IPv6 addresses for comparison", 
                         std::chrono::milliseconds(0));
    }
    
    if (!(addr4.value() == addr5.value())) {
        return TestResult(false, "Equal IPv6 addresses should compare equal", 
                         std::chrono::milliseconds(0));
    }
    
    if (addr4.value() == addr6.value()) {
        return TestResult(false, "Different IPv6 addresses should not compare equal", 
                         std::chrono::milliseconds(0));
    }
    
    return TestResult(true, "", std::chrono::milliseconds(0));
}

inline auto test_performance_benchmark() -> TestResult {
    const int iterations = 10000;
    
    PerformanceTimer timer;
    
    // Benchmark IPv4 parsing
    for (int i = 0; i < iterations; ++i) {
        auto addr = IPAddress::from_string("192.168.1.1");
        if (!addr.has_value()) {
            return TestResult(false, "Benchmark failed due to parsing error", 
                             std::chrono::milliseconds(0));
        }
    }
    
    auto ipv4_duration = timer.elapsed_microseconds();
    
    timer = PerformanceTimer();
    
    // Benchmark IPv6 parsing
    for (int i = 0; i < iterations; ++i) {
        auto addr = IPAddress::from_string("2001:db8::1");
        if (!addr.has_value()) {
            return TestResult(false, "Benchmark failed due to parsing error", 
                             std::chrono::milliseconds(0));
        }
    }
    
    auto ipv6_duration = timer.elapsed_microseconds();
    
    std::cout << "IPv4 parsing: " << (ipv4_duration.count() / iterations) << " μs/op" << std::endl;
    std::cout << "IPv6 parsing: " << (ipv6_duration.count() / iterations) << " μs/op" << std::endl;
    
    return TestResult(true, "Performance benchmark completed", std::chrono::milliseconds(0));
}

inline auto run_ip_address_tests() -> bool {
    TestSuite suite("IPAddress Tests");
    
    suite.add_test("IPv4 Parsing", test_ipv4_parsing);
    suite.add_test("IPv6 Parsing", test_ipv6_parsing);
    suite.add_test("Address Comparison", test_address_comparison);
    suite.add_test("Performance Benchmark", test_performance_benchmark);
    
    return suite.run();
}

} // namespace test
} // namespace dualstack