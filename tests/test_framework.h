#pragma once

/**
 * DualStackNet26 - Amphisbaena 🐍
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * Yorkshire Champion Standards - Improving AI Safety and the Web
 * British Standards improving AI Safety and the Web
 * 
 * Weinberg's Second Law: If builders built buildings the way programmers wrote programs, 
 * then the first woodpecker that came along would destroy civilization.
 */

#include <iostream>
#include <string>
#include <chrono>
#include <functional>
#include <vector>

namespace dualstack {
namespace test {

class TestResult {
public:
    bool passed;
    std::string message;
    std::chrono::milliseconds duration;
    
    TestResult(bool p, std::string msg, std::chrono::milliseconds dur)
        : passed(p), message(std::move(msg)), duration(dur) {}
};

class TestCase {
private:
    std::string name_;
    std::function<TestResult()> test_func_;
    
public:
    TestCase(std::string name, std::function<TestResult()> func)
        : name_(std::move(name)), test_func_(std::move(func)) {}
    
    auto run() -> TestResult {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = test_func_();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        result.duration = duration;
        return result;
    }
    
    auto name() const -> const std::string& { return name_; }
};

class TestSuite {
private:
    std::string name_;
    std::vector<TestCase> tests_;
    std::vector<TestResult> results_;
    
public:
    explicit TestSuite(std::string name) : name_(std::move(name)) {}
    
    auto add_test(std::string name, std::function<TestResult()> test_func) -> void {
        tests_.emplace_back(std::move(name), std::move(test_func));
    }
    
    auto run() -> bool {
        std::cout << "Running test suite: " << name_ << std::endl;
        std::cout << "========================================" << std::endl;
        
        int passed = 0;
        int total = 0;
        
        for (const auto& test : tests_) {
            std::cout << "Running " << test.name() << "... ";
            auto result = test.run();
            
            if (result.passed) {
                std::cout << "PASSED";
                ++passed;
            } else {
                std::cout << "FAILED";
            }
            
            std::cout << " (" << result.duration.count() << "ms)";
            if (!result.message.empty()) {
                std::cout << " - " << result.message;
            }
            std::cout << std::endl;
            
            results_.push_back(result);
            ++total;
        }
        
        std::cout << "========================================" << std::endl;
        std::cout << "Results: " << passed << "/" << total << " tests passed" << std::endl;
        
        return passed == total;
    }
};

// Assertion macros
inline auto assert_true(bool condition, const std::string& message = "") -> TestResult {
    if (condition) {
        return TestResult(true, "", std::chrono::milliseconds(0));
    } else {
        return TestResult(false, "Assertion failed: expected true, got false" + 
                         (message.empty() ? "" : " - " + message), 
                         std::chrono::milliseconds(0));
    }
}

inline auto assert_false(bool condition, const std::string& message = "") -> TestResult {
    if (!condition) {
        return TestResult(true, "", std::chrono::milliseconds(0));
    } else {
        return TestResult(false, "Assertion failed: expected false, got true" + 
                         (message.empty() ? "" : " - " + message), 
                         std::chrono::milliseconds(0));
    }
}

template<typename T>
inline auto assert_equal(const T& actual, const T& expected, const std::string& message = "") -> TestResult {
    if (actual == expected) {
        return TestResult(true, "", std::chrono::milliseconds(0));
    } else {
        return TestResult(false, "Assertion failed: expected " + std::to_string(expected) + 
                         ", got " + std::to_string(actual) + 
                         (message.empty() ? "" : " - " + message), 
                         std::chrono::milliseconds(0));
    }
}

template<typename T>
inline auto assert_not_equal(const T& actual, const T& expected, const std::string& message = "") -> TestResult {
    if (actual != expected) {
        return TestResult(true, "", std::chrono::milliseconds(0));
    } else {
        return TestResult(false, "Assertion failed: expected not equal to " + std::to_string(expected) + 
                         (message.empty() ? "" : " - " + message), 
                         std::chrono::milliseconds(0));
    }
}

// Performance measurement utilities
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start_;
    
public:
    PerformanceTimer() {
        start_ = std::chrono::high_resolution_clock::now();
    }
    
    auto elapsed() -> std::chrono::milliseconds {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
    }
    
    auto elapsed_microseconds() -> std::chrono::microseconds {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    }
};

// Benchmark utilities
template<typename Func>
auto benchmark(const std::string& name, Func&& func, int iterations = 1000) -> void {
    std::cout << "Benchmarking " << name << "... ";
    
    PerformanceTimer timer;
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto elapsed = timer.elapsed_microseconds();
    
    double avg_microseconds = static_cast<double>(elapsed.count()) / iterations;
    double ops_per_second = 1000000.0 / avg_microseconds;
    
    std::cout << "Average: " << avg_microseconds << " μs/op, "
              << ops_per_second << " ops/sec" << std::endl;
}

} // namespace test
} // namespace dualstack