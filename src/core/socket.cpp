#include "socket.h"
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace dualstack {

#ifdef _WIN32
using socklen_t = int;
#endif

// Helper function to convert IPAddress to sockaddr_storage
auto ip_to_sockaddr(const IPAddress& ip, port_t port, sockaddr_storage& addr, socklen_t& addr_len) -> void {
    std::memset(&addr, 0, sizeof(addr));
    
    if (ip.is_ipv4()) {
        auto* addr4 = reinterpret_cast<sockaddr_in*>(&addr);
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        addr4->sin_addr.s_addr = htonl(ip.get_ipv4().address);
        addr_len = sizeof(sockaddr_in);
    } else {
        auto* addr6 = reinterpret_cast<sockaddr_in6*>(&addr);
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        // Convert IPv6 address from our format to in6_addr
        auto ipv6 = ip.get_ipv6();
        std::uint8_t* bytes = reinterpret_cast<std::uint8_t*>(&addr6->sin6_addr);
        for (std::size_t i = 0; i < 8; ++i) {
            bytes[i * 2] = (ipv6.high >> (56 - i * 8)) & 0xFF;
            bytes[i * 2 + 1] = (ipv6.high >> (56 - i * 8 - 8)) & 0xFF;
        }
        for (std::size_t i = 0; i < 8; ++i) {
            bytes[16 + i * 2] = (ipv6.low >> (56 - i * 8)) & 0xFF;
            bytes[16 + i * 2 + 1] = (ipv6.low >> (56 - i * 8 - 8)) & 0xFF;
        }
        addr_len = sizeof(sockaddr_in6);
    }
}

// Helper function to convert sockaddr_storage to IPAddress
auto sockaddr_to_ip(const sockaddr_storage& addr) -> std::expected<IPAddress, error_code> {
    if (addr.ss_family == AF_INET) {
        const auto* addr4 = reinterpret_cast<const sockaddr_in*>(&addr);
        std::uint32_t ip_addr = ntohl(addr4->sin_addr.s_addr);
        return IPAddress(ipv4_address(ip_addr));
    } else if (addr.ss_family == AF_INET6) {
        const auto* addr6 = reinterpret_cast<const sockaddr_in6*>(&addr);
        // Convert in6_addr to our IPv6 format
        const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&addr6->sin6_addr);
        std::uint64_t high = 0;
        std::uint64_t low = 0;
        for (std::size_t i = 0; i < 8; ++i) {
            high |= (static_cast<std::uint64_t>(bytes[i * 2]) << (56 - i * 8));
            high |= (static_cast<std::uint64_t>(bytes[i * 2 + 1]) << (56 - i * 8 - 8));
        }
        for (std::size_t i = 0; i < 8; ++i) {
            low |= (static_cast<std::uint64_t>(bytes[16 + i * 2]) << (56 - i * 8));
            low |= (static_cast<std::uint64_t>(bytes[16 + i * 2 + 1]) << (56 - i * 8 - 8));
        }
        return IPAddress(ipv6_address(high, low));
    }
    return std::unexpected<error_code>(error_code::invalid_address);
}

// Socket implementation
Socket::Socket() : handle_(0), is_open_(false) {
#ifdef _WIN32
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            throw std::runtime_error("Failed to initialize Winsock");
        }
        initialized = true;
    }
#endif
}

Socket::~Socket() {
    if (is_open_) {
        disconnect();
    }
}

Socket::Socket(Socket&& other) noexcept 
    : handle_(other.handle_), is_open_(other.is_open_) {
    other.handle_ = 0;
    other.is_open_ = false;
}

auto Socket::operator=(Socket&& other) noexcept -> Socket& {
    if (this != &other) {
        if (is_open_) {
            disconnect();
        }
        handle_ = other.handle_;
        is_open_ = other.is_open_;
        other.handle_ = 0;
        other.is_open_ = false;
    }
    return *this;
}

auto Socket::connect(const IPAddress& addr, port_t port) -> error_code {
    // Create socket if not already open
    if (!is_open_) {
        handle_ = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (handle_ == static_cast<native_socket_handle>(-1)) {
            return error_code::connection_failed;
        }
        is_open_ = true;
        
        // Enable dual-stack support
        int ipv6_only = 0;
        setsockopt(static_cast<int>(handle_), IPPROTO_IPV6, IPV6_V6ONLY, 
                   reinterpret_cast<const char*>(&ipv6_only), sizeof(ipv6_only));
    }
    
    // Convert address
    sockaddr_storage addr_storage;
    socklen_t addr_len;
    ip_to_sockaddr(addr, port, addr_storage, addr_len);
    
    // Connect
    if (::connect(static_cast<int>(handle_), 
                  reinterpret_cast<const sockaddr*>(&addr_storage), addr_len) == -1) {
        return error_code::connection_failed;
    }
    
    return error_code::success;
}

auto Socket::disconnect() -> void {
    if (is_open_) {
#ifdef _WIN32
        closesocket(static_cast<SOCKET>(handle_));
#else
        ::close(static_cast<int>(handle_));
#endif
        is_open_ = false;
        handle_ = 0;
    }
}

auto Socket::send(buffer_t data) -> std::size_t {
    if (!is_open_) {
        return 0;
    }
    
    auto result = ::send(static_cast<int>(handle_), 
                        reinterpret_cast<const char*>(data.data()), 
                        static_cast<int>(data.size()), 0);
    
    if (result == -1) {
        return 0;
    }
    
    return static_cast<std::size_t>(result);
}

auto Socket::receive(buffer_t buffer) -> std::size_t {
    if (!is_open_) {
        return 0;
    }
    
    auto result = ::recv(static_cast<int>(handle_), 
                        reinterpret_cast<char*>(buffer.data()), 
                        static_cast<int>(buffer.size()), 0);
    
    if (result == -1) {
        return 0;
    }
    
    return static_cast<std::size_t>(result);
}

auto Socket::set_reuse_address(bool reuse) -> error_code {
    if (!is_open_) {
        return error_code::invalid_address;
    }
    
    int optval = reuse ? 1 : 0;
    if (setsockopt(static_cast<int>(handle_), SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&optval), sizeof(optval)) == -1) {
        return error_code::invalid_address;
    }
    
    return error_code::success;
}

auto Socket::set_non_blocking(bool non_blocking) -> error_code {
    if (!is_open_) {
        return error_code::invalid_address;
    }
    
#ifdef _WIN32
    u_long mode = non_blocking ? 1 : 0;
    if (ioctlsocket(static_cast<SOCKET>(handle_), FIONBIO, &mode) != 0) {
        return error_code::invalid_address;
    }
#else
    int flags = fcntl(static_cast<int>(handle_), F_GETFL, 0);
    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(static_cast<int>(handle_), F_SETFL, flags) == -1) {
        return error_code::invalid_address;
    }
#endif
    
    return error_code::success;
}

// Helper functions
auto create_tcp_socket() -> std::expected<Socket, error_code> {
    Socket socket;
    // Socket is created on first connect
    return socket;
}

auto create_udp_socket() -> std::expected<Socket, error_code> {
    // Not implemented yet
    return std::unexpected<error_code>(error_code::invalid_address);
}

} // namespace dualstack