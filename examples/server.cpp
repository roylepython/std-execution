#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

// DualStackNet26 headers
#include "core/ip_address.h"
#include "core/socket.h"
#include "core/acceptor.h"
#include "async/execution.h"
#include "reflect/reflection.h"
#include "security/security.h"
#include "performance/optimization.h"

using namespace dualstack;

// Simple echo server example
class EchoServer {
private:
    Acceptor acceptor_;
    performance::perf_monitor monitor_;
    
public:
    EchoServer(port_t port) : acceptor_(port) {
        std::cout << "Echo server started on port " << port << std::endl;
    }
    
    auto run() -> void {
        std::cout << "Server running... Press Ctrl+C to stop" << std::endl;
        
        while (true) {
            try {
                // Accept incoming connections
                auto client_result = acceptor_.accept();
                if (client_result.has_value()) {
                    monitor_.start_operation();
                    handle_client(client_result.value());
                } else {
                    std::cerr << "Accept failed: " << static_cast<int>(client_result.error()) << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception: " << e.what() << std::endl;
            }
        }
    }
    
private:
    auto handle_client(Socket& client) -> void {
        std::cout << "Client connected" << std::endl;
        
        try {
            // Echo loop
            std::vector<std::byte> buffer(1024);
            while (true) {
                auto received = client.receive(buffer);
                if (received == 0) {
                    break; // Client disconnected
                }
                
                // Echo back
                auto sent = client.send({buffer.data(), received});
                if (sent != received) {
                    break; // Send failed
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Client handling error: " << e.what() << std::endl;
        }
        
        std::cout << "Client disconnected" << std::endl;
    }
};

// Configuration example using reflection
struct ServerConfig {
    port_t port = 8080;
    std::size_t max_connections = 100;
    bool enable_ipv6 = true;
    bool enable_security = true;
    
    // Reflection support
    template<typename Reflection>
    constexpr void reflect(Reflection& r) {
        r.reflect(port, "port");
        r.reflect(max_connections, "max_connections");
        r.reflect(enable_ipv6, "enable_ipv6");
        r.reflect(enable_security, "enable_security");
    }
};

// Main function
auto main() -> int {
    try {
        // Create server configuration
        ServerConfig config;
        
        // Demonstrate reflection capabilities
        std::cout << "Server configuration:" << std::endl;
        std::cout << "  Port: " << config.port << std::endl;
        std::cout << "  Max connections: " << config.max_connections << std::endl;
        std::cout << "  IPv6 enabled: " << (config.enable_ipv6 ? "yes" : "no") << std::endl;
        std::cout << "  Security enabled: " << (config.enable_security ? "yes" : "no") << std::endl;
        
        // Create and run server
        EchoServer server(config.port);
        
        // Performance monitoring
        performance::perf_monitor perf;
        
        // Run server in a separate thread for demonstration
        std::thread server_thread([&server]() {
            server.run();
        });
        
        // Monitor performance
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Performance: " << perf.get_operations_per_second() << " ops/sec" << std::endl;
        }
        
        // Stop server (in a real application, you'd have proper shutdown)
        server_thread.detach();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}