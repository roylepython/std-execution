/**
 * Amphisbaena 🐍 - Async Connection Manager
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * PUBLIC LIBRARY - GAME CHANGER FOR ASYNC DUAL-STACK NETWORKING
 * 
 * Async connection management with std::execution for PsiForceDB, MedusaServ, FCCP, GalaxyCDN
 * 
 * Yorkshire Champion Standards - Improving AI Safety and the Web
 */

#pragma once

// Include format header fix BEFORE any standard headers to prevent GCC 14.2.0 format header bug
#include "../../include/dualstack_net26/fix_format_header.h"
#include "../core/ip_address.h"
#include "../core/socket.h"
#include "../core/acceptor.h"
#include "../async/execution.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>
#include <expected>
#include <span>
#include <execution>
#include <unordered_map>
#include <queue>
#include <condition_variable>

// Public API Export for DLL/SO
#ifdef AMPHISBAENA_BUILDING_LIBRARY
    #ifdef _WIN32
        #define AMPHISBAENA_API __declspec(dllexport)
    #else
        #define AMPHISBAENA_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef _WIN32
        #define AMPHISBAENA_API __declspec(dllimport)
    #else
        #define AMPHISBAENA_API __attribute__((visibility("default")))
    #endif
#endif

namespace dualstack {
namespace network {

// GalaxyCDN Protocol Support
namespace GalaxyCDN {
    struct ProtocolHeader {
        uint32_t magic;           // Protocol magic number
        uint16_t version;         // Protocol version
        uint16_t flags;           // Protocol flags
        uint32_t payload_length;  // Payload length
        uint64_t request_id;      // Request ID for tracking
    };

    constexpr uint32_t PROTOCOL_MAGIC = 0x47414C58; // "GALX"
    constexpr uint16_t PROTOCOL_VERSION = 1;
}

// Connection state tracking
struct ConnectionState {
    std::unique_ptr<Socket> socket;
    IPAddress remote_addr;
    port_t remote_port;
    std::chrono::system_clock::time_point connected_at;
    std::atomic<bool> active{true};
    std::string connection_id;
};

/**
 * @brief Async Connection Manager
 * 
 * Manages asynchronous connections with proper state tracking and lifecycle management.
 * Designed for high-performance dual-stack networking with std::execution.
 */
class AMPHISBAENA_API AsyncConnectionManager {
public:
    AsyncConnectionManager();
    ~AsyncConnectionManager();

    // Initialization
    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }

    // Async connection management
    std::string create_async_connection(const IPAddress& addr, port_t port);
    void close_connection(const std::string& connection_id);
    ConnectionState* get_connection(const std::string& connection_id);
    
    // Async operations using std::execution
#if __cpp_lib_execution >= 202300L
    auto async_accept(Acceptor& acceptor) -> std::execution::sender auto;
    auto async_connect(const IPAddress& addr, port_t port) -> std::execution::sender auto;
    auto async_send(const std::string& connection_id, buffer_t data) -> std::execution::sender auto;
    auto async_receive(const std::string& connection_id, buffer_t buffer) -> std::execution::sender auto;
#endif

    // GalaxyCDN protocol support
    bool send_galaxycdn_message(const std::string& connection_id, const std::vector<std::byte>& payload);
    std::expected<std::vector<std::byte>, error_code> receive_galaxycdn_message(const std::string& connection_id);

    // Connection statistics
    size_t get_active_connection_count() const;
    std::vector<std::string> get_all_connection_ids() const;

    // Get IO context for async operations
    async::io_context* get_io_context() { return io_context_.get(); }

private:
    std::atomic<bool> initialized_;
    std::unique_ptr<async::io_context> io_context_;
    std::mutex connections_mutex_;
    
    // Async connection tracking
    std::unordered_map<std::string, std::unique_ptr<ConnectionState>> active_connections_;
    std::atomic<uint64_t> connection_counter_{0};
    
    std::string generate_connection_id();
};

/**
 * @brief Async Dual-Stack Server
 * 
 * High-performance dual-stack server with async connection handling.
 * Manages both IPv4 and IPv6 listeners simultaneously.
 */
class AMPHISBAENA_API AsyncDualStackServer {
public:
    AsyncDualStackServer(port_t port);
    ~AsyncDualStackServer();

    bool start();
    void stop();
    bool is_running() const { return running_; }

    // Async connection handling
    void set_connection_handler(std::function<void(std::string connection_id, Socket&, const IPAddress&)> handler);
    
    // GalaxyCDN protocol handler
    void set_galaxycdn_handler(
        std::function<void(std::string connection_id, Socket&, const GalaxyCDN::ProtocolHeader&, std::vector<std::byte>)> handler);

    // Connection management
    void close_connection(const std::string& connection_id);
    size_t get_active_connection_count();

    // Get connection manager
    AsyncConnectionManager* get_connection_manager() { return connection_manager_.get(); }

private:
    port_t port_;
    std::atomic<bool> running_;
    std::unique_ptr<Acceptor> ipv4_acceptor_;
    std::unique_ptr<Acceptor> ipv6_acceptor_;
    std::thread ipv4_thread_;
    std::thread ipv6_thread_;
    std::thread async_worker_thread_;
    
    std::function<void(std::string, Socket&, const IPAddress&)> connection_handler_;
    std::function<void(std::string, Socket&, const GalaxyCDN::ProtocolHeader&, std::vector<std::byte>)> galaxycdn_handler_;
    
    std::unique_ptr<AsyncConnectionManager> connection_manager_;
    
    // Async connection queue
    struct PendingConnection {
        std::string connection_id;
        Socket socket;
        IPAddress addr;
        std::chrono::system_clock::time_point accepted_at;
    };
    
    std::mutex pending_mutex_;
    std::queue<PendingConnection> pending_connections_;
    std::condition_variable pending_cv_;
    std::atomic<bool> worker_running_{false};
    
    // Active connections tracking
    std::mutex active_connections_mutex_;
    std::unordered_map<std::string, std::unique_ptr<ConnectionState>> active_connections_;
    std::atomic<uint64_t> connection_counter_{0};

    void handle_ipv4_connections();
    void handle_ipv6_connections();
    void async_worker_loop();
    void handle_client_async(std::string connection_id, Socket client, const IPAddress& addr);
    std::string generate_connection_id();
};

} // namespace network
} // namespace dualstack