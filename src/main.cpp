#include "core/ip_address.h"
#include "core/socket.h"
#include "core/acceptor.h"
#include "async/execution.h"
#include "reflect/reflection.h"
#include "security/security.h"
#include "performance/optimization.h"
#include <iostream>

using namespace dualstack;

auto main() -> int {
    std::cout << "DualStackNet26 Library Compilation Test" << std::endl;
    
    // Test basic functionality
    auto ip_result = IPAddress::from_string("192.168.1.1");
    if (ip_result.has_value()) {
        std::cout << "IPv4 parsing successful: " << ip_result.value().to_string() << std::endl;
    } else {
        std::cout << "IPv4 parsing failed" << std::endl;
        return 1;
    }
    
    auto ipv6_result = IPAddress::from_string("2001:db8::1");
    if (ipv6_result.has_value()) {
        std::cout << "IPv6 parsing successful: " << ipv6_result.value().to_string() << std::endl;
    } else {
        std::cout << "IPv6 parsing failed" << std::endl;
        return 1;
    }
    
    // Test socket creation
    auto socket_result = create_tcp_socket();
    if (socket_result.has_value()) {
        std::cout << "TCP socket creation successful" << std::endl;
    } else {
        std::cout << "TCP socket creation failed" << std::endl;
        return 1;
    }
    
    // Test acceptor creation
    auto acceptor_result = create_acceptor(8080);
    if (acceptor_result.has_value()) {
        std::cout << "Acceptor creation successful" << std::endl;
    } else {
        std::cout << "Acceptor creation failed" << std::endl;
        return 1;
    }
    
    // Test async context
    async::io_context ctx;
    std::cout << "Async context creation successful" << std::endl;
    
    // Test security features
    std::array<IPAddress, 2> blocked_ips = {
        ip_result.value(),
        ipv6_result.value()
    };
    security::AccessControlList acl({blocked_ips.data(), blocked_ips.size()});
    std::cout << "Security ACL creation successful" << std::endl;
    
    // Test performance utilities
    performance::perf_monitor monitor;
    monitor.start_operation();
    std::cout << "Performance monitor test successful" << std::endl;
    
    std::cout << "✅ All basic functionality tests passed!" << std::endl;
    return 0;
}