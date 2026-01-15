#pragma once

// Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved

#include "ip_address.h"
#include <memory>
#include <cstdint>
#include <cstddef>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

// Simple span-like wrapper for buffer operations
template<typename T>
class span_wrapper {
private:
    const T* data_;
    std::size_t size_;
    
public:
    span_wrapper(const T* data, std::size_t size) : data_(data), size_(size) {}
    span_wrapper(const std::vector<std::remove_const_t<T>>& vec) : data_(vec.data()), size_(vec.size()) {}
    
    const T* data() const { return data_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
};

namespace dualstack {

// Forward declarations
class Socket;
class Acceptor;
class IPAddress;

// Type aliases for cleaner API
using port_t = std::uint16_t;
using buffer_t = span_wrapper<const std::byte>;

// Helper function declarations
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socklen_t = int;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
auto ip_to_sockaddr(const IPAddress& ip, port_t port, sockaddr_storage& addr, socklen_t& addr_len) -> void;
auto sockaddr_to_ip(const sockaddr_storage& addr, socklen_t addr_len, IPAddress& ip, port_t& port) -> void;

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
    bool owns_handle_;  // Track if we own the handle (for move semantics)
    
public:
    // Constructors
    Socket();
    explicit Socket(native_socket_handle handle, bool owns = true);  // Construct from native handle
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