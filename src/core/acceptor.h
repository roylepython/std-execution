#pragma once

#include "socket.h"
#include <memory>
#include <expected>

namespace dualstack {

class Acceptor {
private:
    Socket listen_socket_;
    bool is_listening_;
    
public:
    // Constructors
    Acceptor();
    explicit Acceptor(port_t port);
    ~Acceptor();
    
    // Move semantics
    Acceptor(Acceptor&& other) noexcept;
    auto operator=(Acceptor&& other) noexcept -> Acceptor&;
    
    // Delete copy operations
    Acceptor(const Acceptor&) = delete;
    auto operator=(const Acceptor&) = delete;
    
    // Listening operations
    [[nodiscard]] auto listen(port_t port, const IPAddress& bind_addr = {}) -> error_code;
    auto stop_listening() -> void;
    
    // Connection acceptance
    [[nodiscard]] auto accept() -> std::expected<Socket, error_code>;
    
    // Asynchronous operations using std::execution (C++26)
#if __cpp_lib_execution >= 202300L
    // Sender-based async operations
    auto async_accept() -> std::execution::sender auto;
    
    // Listen with async callback
    template<typename Receiver>
    auto async_listen(port_t port, Receiver&& receiver) -> std::execution::sender auto;
#endif
    
    // Utility methods
    bool is_listening() const { return is_listening_; }
    
    // Socket binding helpers
    [[nodiscard]] auto bind_to_interface(const IPAddress& addr) -> error_code;
    [[nodiscard]] auto bind_to_all_interfaces() -> error_code;
    
    // Configuration
    auto set_backlog(int backlog) -> error_code;
    auto enable_dual_stack(bool enable = true) -> error_code;
};

// Helper function for acceptor creation
[[nodiscard]] auto create_acceptor(port_t port) -> std::expected<Acceptor, error_code>;

} // namespace dualstack