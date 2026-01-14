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
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <future>

// DualStackNet26 headers
#include "../src/core/ip_address.h"
#include "../src/core/socket.h"
#include "../src/core/acceptor.h"
#include "../src/async/execution.h"
#include "../src/performance/optimization.h"

using namespace dualstack;

// Concurrent Dual-Stack Server Example
// Demonstrates simultaneous IPv4 and IPv6 connections with async handshakes
class ConcurrentDualStackServer {
private:
    Acceptor ipv4_acceptor_;
    Acceptor ipv6_acceptor_;
    std::atomic<bool> running_;
    performance::perf_monitor perf_monitor_;
    
public:
    ConcurrentDualStackServer(port_t port) 
        : ipv4_acceptor_(port), ipv6_acceptor_(port), running_(true) {
        
        // Enable dual-stack mode for IPv6 acceptor
        ipv6_acceptor_.enable_dual_stack(true);
        
        std::cout << "🚀 Concurrent Dual-Stack Server Started" << std::endl;
        std::cout << "🌐 Listening on port " << port << " for both IPv4 and IPv6" << std::endl;
        std::cout << "📋 Commands: 'quit' to exit, 'stats' for performance stats" << std::endl;
    }
    
    auto run() -> void {
        std::cout << "🔄 Server running... Press Ctrl+C to stop" << std::endl;
        
        // Start both IPv4 and IPv6 listeners concurrently
        auto ipv4_thread = std::thread(&ConcurrentDualStackServer::handle_ipv4_connections, this);
        auto ipv6_thread = std::thread(&ConcurrentDualStackServer::handle_ipv6_connections, this);
        
        // Handle console input for commands
        handle_console_commands();
        
        // Stop server and wait for threads
        running_ = false;
        if (ipv4_thread.joinable()) ipv4_thread.join();
        if (ipv6_thread.joinable()) ipv6_thread.join();
        
        std::cout << "🛑 Server stopped" << std::endl;
    }
    
private:
    auto handle_ipv4_connections() -> void {
        std::cout << "🟡 IPv4 listener started" << std::endl;
        
        while (running_) {
            try {
                auto client_result = ipv4_acceptor_.accept();
                if (client_result.has_value()) {
                    perf_monitor_.start_operation();
                    std::cout << "🔗 IPv4 client connected" << std::endl;
                    
                    // Handle in separate thread for concurrency
                    std::thread client_thread([this, client = std::move(client_result.value())]() mutable {
                        handle_client_connection(std::move(client), "IPv4");
                    });
                    client_thread.detach();
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } catch (const std::exception& e) {
                if (running_) {
                    std::cerr << "🔴 IPv4 accept error: " << e.what() << std::endl;
                }
            }
        }
        std::cout << "🟡 IPv4 listener stopped" << std::endl;
    }
    
    auto handle_ipv6_connections() -> void {
        std::cout << "🟢 IPv6 listener started" << std::endl;
        
        while (running_) {
            try {
                auto client_result = ipv6_acceptor_.accept();
                if (client_result.has_value()) {
                    perf_monitor_.start_operation();
                    std::cout << "🔗 IPv6 client connected" << std::endl;
                    
                    // Handle in separate thread for concurrency
                    std::thread client_thread([this, client = std::move(client_result.value())]() mutable {
                        handle_client_connection(std::move(client), "IPv6");
                    });
                    client_thread.detach();
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } catch (const std::exception& e) {
                if (running_) {
                    std::cerr << "🔴 IPv6 accept error: " << e.what() << std::endl;
                }
            }
        }
        std::cout << "🟢 IPv6 listener stopped" << std::endl;
    }
    
    auto handle_client_connection(Socket client, const std::string& protocol) -> void {
        try {
            std::cout << "🔄 " << protocol << " client handler started" << std::endl;
            
            // Simulate HTTPS handshake (simplified)
            if (perform_https_handshake(client)) {
                std::cout << "🤝 " << protocol << " HTTPS handshake completed" << std::endl;
                
                // Echo loop
                std::vector<std::byte> buffer(1024);
                while (running_ && client.is_open()) {
                    auto received = client.receive(buffer);
                    if (received == 0) break;
                    
                    // Echo back
                    auto sent = client.send({buffer.data(), received});
                    if (sent != received) break;
                }
            }
            
            std::cout << "🔚 " << protocol << " client disconnected" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "❌ " << protocol << " client error: " << e.what() << std::endl;
        }
    }
    
    auto perform_https_handshake(Socket& client) -> bool {
        // Simulate HTTPS handshake with delay
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return true; // Simulate successful handshake
    }
    
    auto handle_console_commands() -> void {
        std::string command;
        std::cout << "⌨️  Enter commands (quit/stats): ";
        
        while (running_ && std::cin >> command) {
            if (command == "quit") {
                std::cout << "👋 Quit command received" << std::endl;
                break;
            } else if (command == "stats") {
                print_stats();
            } else {
                std::cout << "❓ Unknown command: " << command << std::endl;
            }
            std::cout << "⌨️  Enter commands (quit/stats): ";
        }
    }
    
    auto print_stats() -> void {
        std::cout << "📊 Performance Statistics:" << std::endl;
        std::cout << "   Operations/sec: " << perf_monitor_.get_operations_per_second() << std::endl;
        std::cout << "   Elapsed time: " << perf_monitor_.get_elapsed_time().count() << "ms" << std::endl;
    }
};

// Async Dual-Handshake Demo
// Shows two simultaneous async handshakes
class AsyncHandshakeDemo {
public:
    static auto demonstrate_concurrent_handshakes() -> void {
        std::cout << "\n🔄 Async Handshake Demo Started" << std::endl;
        
        // Create two async operations that run simultaneously
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Simulate two concurrent HTTPS handshakes
        auto handshake1 = std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return "Handshake 1 Complete";
        });
        
        auto handshake2 = std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return "Handshake 2 Complete";
        });
        
        // Wait for both to complete
        auto result1 = handshake1.get();
        auto result2 = handshake2.get();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✅ " << result1 << std::endl;
        std::cout << "✅ " << result2 << std::endl;
        std::cout << "⏱️  Both handshakes completed in " << duration.count() << "ms" << std::endl;
        std::cout << "📈 Total handshakes per second: " << (2000.0 / duration.count()) << std::endl;
    }
};

// Main function
auto main() -> int {
    try {
        std::cout << "🧪 DualStackNet26 Concurrent Server Demo" << std::endl;
        std::cout << "======================================" << std::endl;
        
        // Demonstrate concurrent handshakes first
        AsyncHandshakeDemo::demonstrate_concurrent_handshakes();
        
        std::cout << "\n🌐 Starting Dual-Stack Server..." << std::endl;
        
        // Create and run concurrent dual-stack server
        ConcurrentDualStackServer server(8443); // HTTPS-like port
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "💥 Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "✅ Demo completed successfully!" << std::endl;
    return 0;
}