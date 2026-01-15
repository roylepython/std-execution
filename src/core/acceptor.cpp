/**
 * DualStackNet26 - Amphisbaena =
 * Copyright  2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * Yorkshire Champion Standards - Improving AI Safety and the Web
 * British Standards improving AI Safety and the Web
 * 
 * Weinberg's Second Law: If builders built buildings the way programmers wrote programs, 
 * then the first woodpecker that came along would destroy civilization.
 */

// Include format header fix BEFORE any standard headers
#include "../../include/dualstack_net26/fix_format_header.h"
#include "acceptor.h"
#include "socket.h"
#include "ip_address.h"

using ::make_unexpected_value;
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace dualstack {

// Acceptor implementation
Acceptor::Acceptor() : is_listening_(false) {}

Acceptor::Acceptor(port_t port) : is_listening_(false) {
    auto result = listen(port);
    if (result != error_code::success) {
        throw std::runtime_error("Failed to create acceptor");
    }
}

Acceptor::~Acceptor() {
    if (is_listening_) {
        stop_listening();
    }
}

Acceptor::Acceptor(Acceptor&& other) noexcept 
    : listen_socket_(std::move(other.listen_socket_)), 
      is_listening_(other.is_listening_) {
    other.is_listening_ = false;
}

auto Acceptor::operator=(Acceptor&& other) noexcept -> Acceptor& {
    if (this != &other) {
        if (is_listening_) {
            stop_listening();
        }
        listen_socket_ = std::move(other.listen_socket_);
        is_listening_ = other.is_listening_;
        other.is_listening_ = false;
    }
    return *this;
}

auto Acceptor::listen(port_t port, const IPAddress& bind_addr) -> error_code {
    // Create listening socket
    auto socket_result = create_tcp_socket();
    if (!socket_result.has_value()) {
        return socket_result.error();
    }
    
    listen_socket_ = std::move(socket_result.value());
    
    // Enable reuse address
    auto reuse_result = listen_socket_.set_reuse_address(true);
    if (reuse_result != error_code::success) {
        return reuse_result;
    }
    
    // Enable dual-stack support
    auto dual_stack_result = enable_dual_stack(true);
    if (dual_stack_result != error_code::success) {
        return dual_stack_result;
    }
    
    // Bind to address
    sockaddr_storage addr_storage;
    socklen_t addr_len;
    
    if (bind_addr.is_ipv4() || bind_addr.is_ipv6()) {
        ip_to_sockaddr(bind_addr, port, addr_storage, addr_len);
    } else {
        // Bind to all interfaces
        std::memset(&addr_storage, 0, sizeof(addr_storage));
        auto* addr6 = reinterpret_cast<sockaddr_in6*>(&addr_storage);
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        addr6->sin6_addr = in6addr_any;
        addr_len = sizeof(sockaddr_in6);
    }
    
    // Bind socket
    if (::bind(static_cast<int>(listen_socket_.get_native_handle()), 
               reinterpret_cast<const sockaddr*>(&addr_storage), addr_len) == -1) {
        return error_code::bind_failed;
    }
    
    // Start listening
    if (::listen(static_cast<int>(listen_socket_.get_native_handle()), SOMAXCONN) == -1) {
        return error_code::listen_failed;
    }
    
    is_listening_ = true;
    return error_code::success;
}

auto Acceptor::stop_listening() -> void {
    if (is_listening_) {
        listen_socket_.disconnect();
        is_listening_ = false;
    }
}

auto Acceptor::accept() -> std::expected<Socket, error_code> {
    if (!is_listening_) {
        return std::unexpected<error_code>(error_code::invalid_address);
    }
    
    sockaddr_storage addr_storage;
    socklen_t addr_len = sizeof(addr_storage);
    
    auto new_socket_handle = ::accept(static_cast<int>(listen_socket_.get_native_handle()), 
                                     reinterpret_cast<sockaddr*>(&addr_storage), &addr_len);
    
    if (new_socket_handle == static_cast<native_socket_handle>(-1)) {
#ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            return std::unexpected<error_code>(error_code::timeout);
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return std::unexpected<error_code>(error_code::timeout);
        }
#endif
        return std::unexpected<error_code>(error_code::accept_failed);
    }
    
    // Create new socket object from accepted handle
    Socket new_socket(static_cast<native_socket_handle>(new_socket_handle), true);
    
    return new_socket;
}

auto Acceptor::bind_to_interface(const IPAddress& addr [[maybe_unused]]) -> error_code {
    // This would be implemented to bind to specific interfaces
    return error_code::success;
}

auto Acceptor::bind_to_all_interfaces() -> error_code {
    // This would be implemented to bind to all available interfaces
    return error_code::success;
}

auto Acceptor::set_backlog(int backlog [[maybe_unused]]) -> error_code {
    if (!is_listening_) {
        return error_code::invalid_address;
    }
    
    // Note: Changing backlog after listen requires stopping and restarting
    return error_code::success;
}

auto Acceptor::enable_dual_stack(bool enable) -> error_code {
    if (!listen_socket_.is_open()) {
        return error_code::invalid_address;
    }
    
    int ipv6_only = enable ? 0 : 1;
    if (setsockopt(static_cast<int>(listen_socket_.get_native_handle()), 
                   IPPROTO_IPV6, IPV6_V6ONLY, 
                   reinterpret_cast<const char*>(&ipv6_only), sizeof(ipv6_only)) == -1) {
        return error_code::invalid_address;
    }
    
    return error_code::success;
}

// Helper function
auto create_acceptor(port_t port) -> std::expected<Acceptor, error_code> {
    Acceptor acceptor;
    auto result = acceptor.listen(port);
    if (result != error_code::success) {
        return std::unexpected<error_code>(result);
    }
    return acceptor;
}

} // namespace dualstack