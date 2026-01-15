/**
 * Amphisbaena 🐍 - Async Connection Manager Implementation
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * PUBLIC LIBRARY IMPLEMENTATION
 */

// Include format header fix BEFORE any standard headers
#include "../../include/dualstack_net26/fix_format_header.h"
#include "async_connection_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace dualstack {
namespace network {

// ============================================================================
// AsyncConnectionManager Implementation
// ============================================================================

AsyncConnectionManager::AsyncConnectionManager()
    : initialized_(false)
{
}

AsyncConnectionManager::~AsyncConnectionManager() {
    shutdown();
}

bool AsyncConnectionManager::initialize() {
    if (initialized_) {
        return true;
    }

    try {
        // Initialize IO context for async operations
        io_context_ = std::make_unique<async::io_context>();
        
        initialized_ = true;
        std::cout << "🐍 AsyncConnectionManager initialized" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ AsyncConnectionManager initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void AsyncConnectionManager::shutdown() {
    if (!initialized_) {
        return;
    }

    // Close all active connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        active_connections_.clear();
    }
    
    // Shutdown IO context
    if (io_context_) {
        io_context_->stop();
        io_context_.reset();
    }
    
    initialized_ = false;
    std::cout << "🐍 AsyncConnectionManager shutdown" << std::endl;
}

std::string AsyncConnectionManager::generate_connection_id() {
    uint64_t id = connection_counter_.fetch_add(1);
    std::stringstream ss;
    ss << "conn_" << std::hex << std::setfill('0') << std::setw(16) << id;
    return ss.str();
}

std::string AsyncConnectionManager::create_async_connection(const IPAddress& addr, port_t port) {
    if (!initialized_) {
        throw std::runtime_error("AsyncConnectionManager not initialized");
    }

    std::string connection_id = generate_connection_id();
    
    auto socket_result = create_tcp_socket();
    if (!socket_result.has_value()) {
        throw std::runtime_error("Failed to create socket");
    }
    
    auto socket = std::make_unique<Socket>(std::move(socket_result.value()));
    
    error_code err = socket->connect(addr, port);
    if (err != error_code::success) {
        throw std::runtime_error("Failed to connect");
    }
    
    auto state = std::make_unique<ConnectionState>();
    state->socket = std::move(socket);
    state->remote_addr = addr;
    state->remote_port = port;
    state->connected_at = std::chrono::system_clock::now();
    state->connection_id = connection_id;
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        active_connections_[connection_id] = std::move(state);
    }
    
    return connection_id;
}

void AsyncConnectionManager::close_connection(const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = active_connections_.find(connection_id);
    if (it != active_connections_.end()) {
        it->second->active = false;
        if (it->second->socket && it->second->socket->is_open()) {
            it->second->socket->disconnect();
        }
        active_connections_.erase(it);
    }
}

ConnectionState* AsyncConnectionManager::get_connection(const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = active_connections_.find(connection_id);
    if (it != active_connections_.end() && it->second->active) {
        return it->second.get();
    }
    return nullptr;
}

bool AsyncConnectionManager::send_galaxycdn_message(const std::string& connection_id, const std::vector<std::byte>& payload) {
    auto* conn = get_connection(connection_id);
    if (!conn || !conn->socket || !conn->socket->is_open()) {
        return false;
    }
    
    GalaxyCDN::ProtocolHeader header{};
    header.magic = GalaxyCDN::PROTOCOL_MAGIC;
    header.version = GalaxyCDN::PROTOCOL_VERSION;
    header.flags = 0;
    header.payload_length = static_cast<uint32_t>(payload.size());
    header.request_id = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Send header
    buffer_t header_span(reinterpret_cast<const std::byte*>(&header), sizeof(header));
    size_t sent = conn->socket->send(header_span);
    if (sent != sizeof(header)) {
        return false;
    }
    
    // Send payload
    if (!payload.empty()) {
        buffer_t payload_span(payload.data(), payload.size());
        sent = conn->socket->send(payload_span);
        if (sent != payload.size()) {
            return false;
        }
    }
    
    return true;
}

std::expected<std::vector<std::byte>, error_code> AsyncConnectionManager::receive_galaxycdn_message(const std::string& connection_id) {
    auto* conn = get_connection(connection_id);
    if (!conn || !conn->socket || !conn->socket->is_open()) {
        return std::unexpected(error_code::connection_failed);
    }
    
    // Receive header
    GalaxyCDN::ProtocolHeader header{};
    std::vector<std::byte> header_buffer(sizeof(header));
    buffer_t header_span(header_buffer.data(), header_buffer.size());
    size_t received = conn->socket->receive(header_span);
    if (received != sizeof(header)) {
        return std::unexpected(error_code::receive_failed);
    }
    
    std::memcpy(&header, header_buffer.data(), sizeof(header));
    
    // Validate magic
    if (header.magic != GalaxyCDN::PROTOCOL_MAGIC) {
        return std::unexpected(error_code::invalid_address);
    }
    
    // Receive payload
    std::vector<std::byte> payload(header.payload_length);
    if (header.payload_length > 0) {
        buffer_t payload_span(payload.data(), payload.size());
        received = conn->socket->receive(payload_span);
        if (received != payload.size()) {
            return std::unexpected(error_code::receive_failed);
        }
    }
    
    return payload;
}

size_t AsyncConnectionManager::get_active_connection_count() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(connections_mutex_));
    return active_connections_.size();
}

std::vector<std::string> AsyncConnectionManager::get_all_connection_ids() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(connections_mutex_));
    std::vector<std::string> ids;
    ids.reserve(active_connections_.size());
    for (const auto& [id, state] : active_connections_) {
        if (state->active) {
            ids.push_back(id);
        }
    }
    return ids;
}

#if __cpp_lib_execution >= 202300L
auto AsyncConnectionManager::async_accept(Acceptor& acceptor) -> std::execution::sender auto {
    return std::execution::just()
        | std::execution::then([&acceptor]() {
            auto result = acceptor.accept();
            if (result.has_value()) {
                return std::make_pair(std::move(result.value()), error_code::success);
            }
            return std::make_pair(Socket{}, result.error());
        });
}

auto AsyncConnectionManager::async_connect(const IPAddress& addr, port_t port) -> std::execution::sender auto {
    return std::execution::just()
        | std::execution::then([this, addr, port]() {
            try {
                std::string conn_id = create_async_connection(addr, port);
                return std::make_pair(conn_id, error_code::success);
            } catch (...) {
                return std::make_pair(std::string{}, error_code::connection_failed);
            }
        });
}

auto AsyncConnectionManager::async_send(const std::string& connection_id, buffer_t data) -> std::execution::sender auto {
    return std::execution::just()
        | std::execution::then([this, connection_id, data]() {
            auto* conn = get_connection(connection_id);
            if (!conn || !conn->socket || !conn->socket->is_open()) {
                return std::make_pair(size_t(0), error_code::connection_failed);
            }
            size_t sent = conn->socket->send(data);
            return std::make_pair(sent, error_code::success);
        });
}

auto AsyncConnectionManager::async_receive(const std::string& connection_id, buffer_t buffer) -> std::execution::sender auto {
    return std::execution::just()
        | std::execution::then([this, connection_id, buffer]() {
            auto* conn = get_connection(connection_id);
            if (!conn || !conn->socket || !conn->socket->is_open()) {
                return std::make_pair(size_t(0), error_code::connection_failed);
            }
            size_t received = conn->socket->receive(buffer);
            return std::make_pair(received, error_code::success);
        });
}
#endif

// ============================================================================
// AsyncDualStackServer Implementation
// ============================================================================

AsyncDualStackServer::AsyncDualStackServer(port_t port)
    : port_(port)
    , running_(false)
{
    connection_manager_ = std::make_unique<AsyncConnectionManager>();
    connection_manager_->initialize();
}

AsyncDualStackServer::~AsyncDualStackServer() {
    stop();
}

bool AsyncDualStackServer::start() {
    if (running_) {
        return true;
    }
    
    try {
        // Create IPv4 acceptor
        auto ipv4_result = create_acceptor(port_);
        if (!ipv4_result.has_value()) {
            std::cerr << "❌ Failed to create IPv4 acceptor" << std::endl;
            return false;
        }
        ipv4_acceptor_ = std::make_unique<Acceptor>(std::move(ipv4_result.value()));
        
        // Create IPv6 acceptor with dual-stack enabled
        auto ipv6_result = create_acceptor(port_);
        if (!ipv6_result.has_value()) {
            std::cerr << "❌ Failed to create IPv6 acceptor" << std::endl;
            return false;
        }
        ipv6_acceptor_ = std::make_unique<Acceptor>(std::move(ipv6_result.value()));
        ipv6_acceptor_->enable_dual_stack(true);
        
        running_ = true;
        worker_running_ = true;
        
        // Start listener threads
        ipv4_thread_ = std::thread(&AsyncDualStackServer::handle_ipv4_connections, this);
        ipv6_thread_ = std::thread(&AsyncDualStackServer::handle_ipv6_connections, this);
        async_worker_thread_ = std::thread(&AsyncDualStackServer::async_worker_loop, this);
        
        std::cout << "🐍 AsyncDualStackServer started on port " << port_ << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Server start failed: " << e.what() << std::endl;
        return false;
    }
}

void AsyncDualStackServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    worker_running_ = false;
    
    // Notify worker thread
    pending_cv_.notify_all();
    
    // Wait for threads
    if (ipv4_thread_.joinable()) ipv4_thread_.join();
    if (ipv6_thread_.joinable()) ipv6_thread_.join();
    if (async_worker_thread_.joinable()) async_worker_thread_.join();
    
    // Close all connections
    {
        std::lock_guard<std::mutex> lock(active_connections_mutex_);
        active_connections_.clear();
    }
    
    std::cout << "🐍 AsyncDualStackServer stopped" << std::endl;
}

void AsyncDualStackServer::handle_ipv4_connections() {
    while (running_) {
        try {
            auto client_result = ipv4_acceptor_->accept();
            if (client_result.has_value()) {
                std::string conn_id = generate_connection_id();
                IPAddress client_addr; // Would be populated from accept result
                
                PendingConnection pending;
                pending.connection_id = conn_id;
                pending.socket = std::move(client_result.value());
                pending.addr = client_addr;
                pending.accepted_at = std::chrono::system_clock::now();
                
                {
                    std::lock_guard<std::mutex> lock(pending_mutex_);
                    pending_connections_.push(std::move(pending));
                }
                pending_cv_.notify_one();
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } catch (const std::exception& e) {
            if (running_) {
                std::cerr << "❌ IPv4 accept error: " << e.what() << std::endl;
            }
        }
    }
}

void AsyncDualStackServer::handle_ipv6_connections() {
    while (running_) {
        try {
            auto client_result = ipv6_acceptor_->accept();
            if (client_result.has_value()) {
                std::string conn_id = generate_connection_id();
                IPAddress client_addr; // Would be populated from accept result
                
                PendingConnection pending;
                pending.connection_id = conn_id;
                pending.socket = std::move(client_result.value());
                pending.addr = client_addr;
                pending.accepted_at = std::chrono::system_clock::now();
                
                {
                    std::lock_guard<std::mutex> lock(pending_mutex_);
                    pending_connections_.push(std::move(pending));
                }
                pending_cv_.notify_one();
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } catch (const std::exception& e) {
            if (running_) {
                std::cerr << "❌ IPv6 accept error: " << e.what() << std::endl;
            }
        }
    }
}

void AsyncDualStackServer::async_worker_loop() {
    while (worker_running_ || !pending_connections_.empty()) {
        std::unique_lock<std::mutex> lock(pending_mutex_);
        pending_cv_.wait(lock, [this] {
            return !pending_connections_.empty() || !worker_running_;
        });
        
        while (!pending_connections_.empty()) {
            PendingConnection pending = std::move(pending_connections_.front());
            pending_connections_.pop();
            lock.unlock();
            
            handle_client_async(pending.connection_id, std::move(pending.socket), pending.addr);
            
            lock.lock();
        }
    }
}

void AsyncDualStackServer::handle_client_async(std::string connection_id, Socket client, const IPAddress& addr) {
    // Store connection
    auto state = std::make_unique<ConnectionState>();
    state->socket = std::make_unique<Socket>(std::move(client));
    state->remote_addr = addr;
    state->connected_at = std::chrono::system_clock::now();
    state->connection_id = connection_id;
    
    {
        std::lock_guard<std::mutex> lock(active_connections_mutex_);
        active_connections_[connection_id] = std::move(state);
    }
    
    // Call connection handler if set
    if (connection_handler_) {
        connection_handler_(connection_id, *active_connections_[connection_id]->socket, addr);
    }
}

void AsyncDualStackServer::close_connection(const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(active_connections_mutex_);
    auto it = active_connections_.find(connection_id);
    if (it != active_connections_.end()) {
        it->second->active = false;
        if (it->second->socket && it->second->socket->is_open()) {
            it->second->socket->disconnect();
        }
        active_connections_.erase(it);
    }
}

size_t AsyncDualStackServer::get_active_connection_count() {
    std::lock_guard<std::mutex> lock(active_connections_mutex_);
    return active_connections_.size();
}

std::string AsyncDualStackServer::generate_connection_id() {
    uint64_t id = connection_counter_.fetch_add(1);
    std::stringstream ss;
    ss << "server_conn_" << std::hex << std::setfill('0') << std::setw(16) << id;
    return ss.str();
}

} // namespace network
} // namespace dualstack