#pragma once

// Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved

#include "ip_address.h"
#include <span>
#include <memory>
#include <cstdint>

// Forward declarations for std::execution types
namespace std::execution {
    struct sender_tag {};
    template<typename... Ts>
    struct completion_signatures {};
    
    struct set_value_t {};
    struct set_error_t {};
    struct set_stopped_t {};
    
    template<typename T>
    struct just_sender;
}

namespace dualstack {

// Forward declarations
class Socket;
class Acceptor;

// Type aliases for cleaner API
using port_t = std::uint16_t;
using buffer_t = std::span<std::byte>;

// Native socket handle (platform-specific)
#ifdef _WIN32
using native_socket_handle = std::uintptr_t;
#else
using native_socket_handle = int;
#endif

// Error codes
enum class error_code : int {
    success = 0,
    connection_failed = 1,
    bind_failed = 2,
    listen_failed = 3,
    accept_failed = 4,
    send_failed = 5,
    receive_failed = 6,
    invalid_address = 7,
    timeout = 8
};

class Socket {
private:
    native_socket_handle handle_;
    bool is_open_;
    
public:
    // Constructors
    Socket();
    ~Socket();
    
    // Move semantics
    Socket(Socket&& other) noexcept;
    auto operator=(Socket&& other) noexcept -> Socket&;
    
    // Delete copy operations
    Socket(const Socket&) = delete;
    auto operator=(const Socket&) = delete;
    
    // Connection operations
    [[nodiscard]] auto connect(const IPAddress& addr, port_t port) -> error_code;
    auto disconnect() -> void;
    
    // Data transfer
    [[nodiscard]] auto send(buffer_t data) -> std::size_t;
    [[nodiscard]] auto receive(buffer_t buffer) -> std::size_t;
    
    // Asynchronous operations using std::execution (C++26)
#if __cpp_lib_execution >= 202300L
    // Sender-based async operations
    auto async_connect(const IPAddress& addr, port_t port) 
        -> std::execution::sender auto;
        
    auto async_send(buffer_t data)
        -> std::execution::sender auto;
        
    auto async_receive(buffer_t buffer)
        -> std::execution::sender auto;
#endif
    
    // Utility methods
    bool is_open() const { return is_open_; }
    auto get_native_handle() const -> native_socket_handle { return handle_; }
    
    // Socket options
    auto set_reuse_address(bool reuse) -> error_code;
    auto set_non_blocking(bool non_blocking) -> error_code;
};

// Helper functions for socket creation
[[nodiscard]] auto create_tcp_socket() -> std::expected<Socket, error_code>;
[[nodiscard]] auto create_udp_socket() -> std::expected<Socket, error_code>;

} // namespace dualstack