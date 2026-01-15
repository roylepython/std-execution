#pragma once

// Copyright  2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved

#include "../core/socket.h"
#include "../core/acceptor.h"
#include <execution>
#include <functional>
#include <chrono>
#include <thread>

namespace dualstack::async {

// Execution context for networking operations
class io_context {
private:
    class impl;
    std::unique_ptr<impl> impl_;
    
public:
    io_context();
    ~io_context();
    
    // Move semantics
    io_context(io_context&& other) noexcept;
    auto operator=(io_context&& other) noexcept -> io_context&;
    
    // Delete copy operations
    io_context(const io_context&) = delete;
    auto operator=(const io_context&) = delete;
    
    // Run the event loop
    auto run() -> void;
    auto run_for(std::chrono::milliseconds timeout) -> void;
    auto run_until_stopped() -> void;
    auto stop() -> void;
    
    // Get scheduler for this context
    // Returns a scheduler that can be used with std::execution algorithms
    // In C++26, we use the inline scheduler by default
#if __cpp_lib_execution >= 202300L
    auto get_scheduler() {
        return std::execution::get_inline_scheduler();
    }
#else
    // Fallback for compilers without std::execution
    void* get_scheduler() { return nullptr; }
#endif
};

// Convenience functions for async operations
#if __cpp_lib_execution >= 202300L

// Async connect operation
template<typename Scheduler>
auto async_connect(Scheduler&& sched, const IPAddress& addr, port_t port)
    -> std::execution::sender auto;

// Async send operation
template<typename Scheduler>
auto async_send(Scheduler&& sched, Socket& socket, buffer_t data)
    -> std::execution::sender auto;

// Async receive operation
template<typename Scheduler>
auto async_receive(Scheduler&& sched, Socket& socket, buffer_t buffer)
    -> std::execution::sender auto;

// Async accept operation
template<typename Scheduler>
auto async_accept(Scheduler&& sched, Acceptor& acceptor)
    -> std::execution::sender auto;

// Timer operations
template<typename Scheduler>
auto async_sleep(Scheduler&& sched, std::chrono::milliseconds duration)
    -> std::execution::sender auto;

// Parallel operations for load balancing
template<typename Scheduler, typename Range, typename Function>
auto async_for_each(Scheduler&& sched, Range&& range, Function&& func)
    -> std::execution::sender auto;

template<typename Scheduler, typename Range, typename Function>
auto async_transform(Scheduler&& sched, Range&& range, Function&& func)
    -> std::execution::sender auto;

#endif

// Utility types for networking
struct network_traits {
    static constexpr bool is_networking_sender = true;
};

// Stop token support for cancellation
// Note: std::execution::stop_token doesn't exist in C++26 yet
// Using std::stop_token from <stop_token> instead
#include <stop_token>
using stop_token = std::stop_token;
using stop_source = std::stop_source;

// Environment queries for networking
template<typename Env>
auto get_network_context(const Env& env) -> io_context*;

} // namespace dualstack::async