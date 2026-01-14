#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <future>
#include <memory>

// DualStackNet26 headers
#include "../src/core/ip_address.h"
#include "../src/core/socket.h"
#include "../src/core/acceptor.h"
#include "../src/async/execution.h"
#include "../src/security/tls_protocol.h"

using namespace dualstack;
using namespace dualstack::security::tls;

// Secure Dual-Stack Server Example
// Demonstrates IPv4/IPv6 with TLS 1.3, PQC, JWT, Icewall
class SecureDualStackServer {
private:
    Acceptor ipv4_acceptor_;
    Acceptor ipv6_acceptor_;
    std::atomic<bool> running_;
    TLSContext tls_context_;
    performance::perf_monitor perf_monitor_;
    
public:
    SecureDualStackServer(port_t port) 
        : ipv4_acceptor_(port), ipv6_acceptor_(port), running_(true) {
        
        // Configure TLS context with PQC support
        TLSConfiguration config;
        config.min_version = Version::TLS_1_2;
        config.max_version = Version::TLS_1_3_PQC;
        config.preferred_suites = {
            CipherSuite::TLS_KYBER768_AES256_GCM_SHA384,
            CipherSuite::TLS_DILITHIUM3_AES256_GCM_SHA384,
            CipherSuite::TLS_AES_256_GCM_SHA384
        };
        config.require_pqc = true;
        config.enable_icewall = true;
        
        tls_context_.set_configuration(config);
        
        // Enable dual-stack mode for IPv6 acceptor
        ipv6_acceptor_.enable_dual_stack(true);
        
        std::cout << "🛡️  Secure Dual-Stack Server Started" << std::endl;
        std::cout << "🌐 Listening on port " << port << " for both IPv4 and IPv6" << std::endl;
        std::cout << "🔒 TLS 1.3 with Post-Quantum Cryptography Enabled" << std::endl;
        std::cout << "🧊 Icewall Security Integration Active" << std::endl;
        std::cout << "📋 Commands: 'quit' to exit, 'stats' for performance stats" << std::endl;
    }
    
    auto run() -> void {
        std::cout << "🔄 Server running... Press Ctrl+C to stop" << std::endl;
        
        // Start both IPv4 and IPv6 listeners concurrently
        auto ipv4_thread = std::thread(&SecureDualStackServer::handle_ipv4_connections, this);
        auto ipv6_thread = std::thread(&SecureDualStackServer::handle_ipv6_connections, this);
        
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
                        handle_secure_client_connection(std::move(client), "IPv4");
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
                        handle_secure_client_connection(std::move(client), "IPv6");
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
    
    auto handle_secure_client_connection(Socket client, const std::string& protocol) -> void {
        try {
            std::cout << "🔄 " << protocol << " secure client handler started" << std::endl;
            
            // Create TLS secure socket
            auto tls_socket = tls_context_.create_secure_socket(IPAddress::from_string("0.0.0.0").value(), 0);
            
            // Perform TLS handshake with PQC
            if (tls_socket->perform_handshake()) {
                std::cout << "🤝 " << protocol << " TLS handshake completed" << std::endl;
                
                // Negotiate post-quantum cryptography
                if (tls_socket->negotiate_post_quantum()) {
                    std::cout << "🔐 " << protocol << " Post-Quantum Cryptography Active" << std::endl;
                }
                
                // Enable Icewall protection
                if (tls_socket->enable_icewall_protection()) {
                    std::cout << "🧊 " << protocol << " Icewall Protection Enabled" << std::endl;
                }
                
                // Simulate JWT authentication
                auto jwt_token = JWTToken::create(
                    "user@example.com",
                    std::chrono::seconds(3600), // 1 hour expiry
                    {{"role", "admin"}, {"scope", "read_write"}}
                );
                
                if (tls_socket->authenticate_with_jwt(jwt_token)) {
                    std::cout << "✅ " << protocol << " JWT Authentication Successful" << std::endl;
                    
                    // Secure communication loop
                    std::vector<std::byte> buffer(1024);
                    while (running_ && client.is_open()) {
                        auto received = client.receive(buffer);
                        if (received == 0) break;
                        
                        // Echo back securely
                        auto sent = tls_socket->secure_send({buffer.data(), received});
                        if (sent == 0) break;
                    }
                } else {
                    std::cout << "❌ " << protocol << " JWT Authentication Failed" << std::endl;
                }
            } else {
                std::cout << "❌ " << protocol << " TLS Handshake Failed" << std::endl;
            }
            
            std::cout << "🔚 " << protocol << " secure client disconnected" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "❌ " << protocol << " secure client error: " << e.what() << std::endl;
        }
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
        std::cout << "   TLS Handshake Performance: " << tls_context_.get_handshake_performance() << " handshakes/sec" << std::endl;
        std::cout << "   Current Sessions: " << tls_context_.get_current_sessions() << std::endl;
    }
};

// Concurrent PQC Handshake Demo
class PqcHandshakeDemo {
public:
    static auto demonstrate_concurrent_pqc_handshakes() -> void {
        std::cout << "\n🔄 Post-Quantum Cryptography Handshake Demo" << std::endl;
        
        // Generate Kyber keypair
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Simulate two concurrent PQC handshakes
        auto handshake1 = std::async(std::launch::async, []() {
            try {
                auto keypair = pqc::KyberKeyExchange::generate_keypair();
                auto encapsulation = pqc::KyberKeyExchange::encapsulate(keypair.first);
                auto decapsulation = pqc::KyberKeyExchange::decapsulate(encapsulation.first, keypair.second);
                
                // Verify shared secrets match
                bool match = (encapsulation.second == decapsulation);
                return std::make_pair("Kyber Handshake 1 Complete", match);
            } catch (...) {
                return std::make_pair("Kyber Handshake 1 Failed", false);
            }
        });
        
        auto handshake2 = std::async(std::launch::async, []() {
            try {
                auto keypair = pqc::DilithiumSignature::generate_keypair();
                std::vector<std::byte> message(32);
                auto signature = pqc::DilithiumSignature::sign(message, keypair.second);
                bool verified = pqc::DilithiumSignature::verify(message, signature, keypair.first);
                return std::make_pair("Dilithium Handshake 2 Complete", verified);
            } catch (...) {
                return std::make_pair("Dilithium Handshake 2 Failed", false);
            }
        });
        
        // Wait for both to complete
        auto result1 = handshake1.get();
        auto result2 = handshake2.get();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✅ " << result1.first << (result1.second ? " - SUCCESS" : " - FAILED") << std::endl;
        std::cout << "✅ " << result2.first << (result2.second ? " - SUCCESS" : " - FAILED") << std::endl;
        std::cout << "⏱️  Both PQC handshakes completed in " << duration.count() << "ms" << std::endl;
    }
};

// Icewall Security Demo
class IcewallSecurityDemo {
public:
    static auto demonstrate_icewall_protection() -> void {
        std::cout << "\n🧊 Icewall Security Integration Demo" << std::endl;
        
        // Test threat monitoring
        std::vector<std::string> test_ips = {"192.168.1.1", "192.168.1.100", "10.0.0.1"};
        
        for (const auto& ip : test_ips) {
            auto threat_level = IcewallProtection::monitor_connection(ip);
            std::cout << "🔍 IP " << ip << " - Threat Level: " << static_cast<int>(threat_level) << std::endl;
            
            if (threat_level != IcewallProtection::ThreatLevel::LOW) {
                IcewallProtection::block_ip(ip, threat_level);
            }
        }
        
        // Log security events
        IcewallProtection::log_security_event({
            IcewallProtection::ThreatLevel::MEDIUM,
            "Suspicious connection attempt detected",
            std::chrono::system_clock::now(),
            "192.168.1.100"
        });
        
        std::cout << "✅ Icewall Security Demo Completed" << std::endl;
    }
};

// JWT Authentication Demo
class JwtAuthDemo {
public:
    static auto demonstrate_jwt_authentication() -> void {
        std::cout << "\n🎫 JWT Authentication Demo" << std::endl;
        
        // Create JWT token
        auto token = JWTToken::create(
            "test.user@domain.com",
            std::chrono::seconds(1800), // 30 minutes
            {
                {"role", "administrator"},
                {"permissions", "read,write,delete"},
                {"department", "engineering"}
            }
        );
        
        std::cout << "✅ JWT Token Created: " << token.to_string() << std::endl;
        std::cout << "👤 Subject: " << token.get_subject() << std::endl;
        std::cout << "⏰ Expires: " << std::chrono::duration_cast<std::chrono::seconds>(
            token.get_expiry().time_since_epoch()).count() << " seconds" << std::endl;
        std::cout << "✅ Token Valid: " << (token.is_expired() ? "No" : "Yes") << std::endl;
    }
};

// Main function
auto main() -> int {
    try {
        std::cout << "🧪 DualStackNet26 Secure Server Demo" << std::endl;
        std::cout << "===================================" << std::endl;
        
        // Demonstrate security features
        PqcHandshakeDemo::demonstrate_concurrent_pqc_handshakes();
        IcewallSecurityDemo::demonstrate_icewall_protection();
        JwtAuthDemo::demonstrate_jwt_authentication();
        
        std::cout << "\n🌐 Starting Secure Dual-Stack Server..." << std::endl;
        
        // Create and run secure dual-stack server
        SecureDualStackServer server(8443); // HTTPS-like port
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "💥 Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "✅ Secure Demo completed successfully!" << std::endl;
    return 0;
}