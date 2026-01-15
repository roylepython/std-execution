/**
 * Amphisbaena 🐍 - Virtual Network Adapter System
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * ENTERPRISE-GRADE VIRTUAL NETWORK ADAPTER SYSTEM
 * 
 * Features:
 * - Real adapter as gateway with user-controlled mapping
 * - Internal virtual network adapters and hubs
 * - Dual-stack IPv4/IPv6 simultaneous support
 * - VPC creation (10.0.0.x) with gateway routing
 * - Full DNS support (Google DNS + custom)
 * - IPv4/IPv6 validation and linking
 * - Network card/hub management
 */

#pragma once

#include "../fix_format_header.h"
#include "../../../src/core/ip_address.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <optional>
#include <expected>
#include <functional>

namespace dualstack {
namespace network {

// Forward declarations
class VirtualAdapter;
class VirtualHub;
class NetworkGateway;

// Adapter types
enum class AdapterType {
    REAL,           // Physical network adapter
    VIRTUAL,        // Virtual network adapter
    BRIDGE,         // Bridge adapter (like WSL)
    TUNNEL          // Tunnel adapter
};

// Adapter state
enum class AdapterState {
    DISABLED,
    ENABLED,
    CONNECTING,
    CONNECTED,
    ERROR
};

// DNS server configuration
struct DNSServer {
    IPAddress address;
    std::string name;  // e.g., "Google DNS", "Cloudflare"
    bool is_ipv6;
    int priority;      // Lower = higher priority
};

// Virtual adapter configuration
struct VirtualAdapterConfig {
    std::string name;
    std::string description;
    AdapterType type;
    
    // IPv4 configuration
    std::optional<ipv4_address> ipv4_addr;
    std::optional<ipv4_address> ipv4_subnet_mask;
    std::optional<ipv4_address> ipv4_gateway;
    
    // IPv6 configuration
    std::optional<ipv6_address> ipv6_addr;
    std::optional<int> ipv6_prefix_length;
    std::optional<ipv6_address> ipv6_gateway;
    
    // DNS configuration
    std::vector<DNSServer> dns_servers;
    
    // VPC configuration
    bool is_vpc_gateway;
    std::optional<std::string> vpc_id;
    
    // Routing
    bool enable_nat;
    bool enable_dhcp;
    bool enable_ipv6_autoconfig;
};

// Network interface information
struct NetworkInterface {
    std::string name;
    std::string description;
    AdapterType type;
    AdapterState state;
    
    // Real adapter info
    std::string mac_address;
    std::string driver_name;
    bool is_physical;
    
    // IP addresses
    std::vector<ipv4_address> ipv4_addresses;
    std::vector<ipv6_address> ipv6_addresses;
    
    // Gateway info
    std::optional<ipv4_address> ipv4_gateway;
    std::optional<ipv6_address> ipv6_gateway;
    
    // DNS
    std::vector<DNSServer> dns_servers;
    
    // Statistics
    std::uint64_t bytes_sent;
    std::uint64_t bytes_received;
    std::uint64_t packets_sent;
    std::uint64_t packets_received;
};

// Virtual Hub - Internal network hub for connecting multiple adapters
class VirtualHub {
private:
    std::string hub_id_;
    std::string name_;
    std::vector<std::string> connected_adapter_ids_;
    mutable std::mutex hub_mutex_;
    
    // Hub routing table
    std::map<ipv4_address, std::string> ipv4_routing_table_;
    std::map<ipv6_address, std::string> ipv6_routing_table_;
    
public:
    VirtualHub(const std::string& hub_id, const std::string& name);
    ~VirtualHub();
    
    // Hub management
    auto add_adapter(const std::string& adapter_id) -> bool;
    auto remove_adapter(const std::string& adapter_id) -> bool;
    auto get_connected_adapters() const -> std::vector<std::string>;
    
    // Routing
    auto route_ipv4(const ipv4_address& dest) -> std::optional<std::string>;
    auto route_ipv6(const ipv6_address& dest) -> std::optional<std::string>;
    auto add_route_ipv4(const ipv4_address& dest, const std::string& adapter_id) -> void;
    auto add_route_ipv6(const ipv6_address& dest, const std::string& adapter_id) -> void;
    
    // Hub info
    auto get_hub_id() const -> const std::string& { return hub_id_; }
    auto get_name() const -> const std::string& { return name_; }
};

// Network Gateway - Manages real adapter as gateway
class NetworkGateway {
private:
    std::string gateway_id_;
    std::string real_adapter_name_;
    NetworkInterface real_adapter_info_;
    mutable std::mutex gateway_mutex_;
    
    // Virtual adapters using this gateway
    std::vector<std::string> virtual_adapter_ids_;
    
    // Gateway routing
    std::map<ipv4_address, std::string> ipv4_gateway_routes_;
    std::map<ipv6_address, std::string> ipv6_gateway_routes_;
    
    // NAT tables (for IPv4)
    std::map<ipv4_address, ipv4_address> nat_ipv4_table_;
    
    // DNS forwarding
    std::vector<DNSServer> dns_servers_;
    bool use_google_dns_;
    
public:
    NetworkGateway(const std::string& real_adapter_name);
    ~NetworkGateway();
    
    // Gateway configuration
    auto initialize() -> std::expected<void, std::string>;
    auto shutdown() -> void;
    auto is_initialized() const -> bool;
    
    // Real adapter management
    auto get_real_adapter_info() const -> NetworkInterface;
    auto set_as_gateway() -> std::expected<void, std::string>;
    
    // Virtual adapter registration
    auto register_virtual_adapter(const std::string& adapter_id) -> bool;
    auto unregister_virtual_adapter(const std::string& adapter_id) -> bool;
    
    // Routing
    auto route_packet_ipv4(const ipv4_address& dest, const ipv4_address& src) -> std::optional<std::string>;
    auto route_packet_ipv6(const ipv6_address& dest, const ipv6_address& src) -> std::optional<std::string>;
    
    // NAT
    auto translate_nat_ipv4(const ipv4_address& private_ip) -> std::optional<ipv4_address>;
    
    // DNS
    auto add_dns_server(const DNSServer& server) -> void;
    auto set_google_dns(bool enable) -> void;
    auto resolve_dns(const std::string& hostname, bool prefer_ipv6 = false) -> std::expected<IPAddress, std::string>;
    
    // Gateway info
    auto get_gateway_id() const -> const std::string& { return gateway_id_; }
};

// Virtual Network Adapter Manager - Main interface for managing virtual adapters
class VirtualAdapterManager {
private:
    mutable std::mutex manager_mutex_;
    
    // Adapters
    std::map<std::string, std::unique_ptr<VirtualAdapter>> adapters_;
    std::map<std::string, std::unique_ptr<VirtualHub>> hubs_;
    std::unique_ptr<NetworkGateway> gateway_;
    
    // VPC management
    std::map<std::string, std::vector<std::string>> vpc_adapters_;  // vpc_id -> adapter_ids
    
    // Adapter counter
    std::atomic<std::uint64_t> adapter_counter_;
    
public:
    VirtualAdapterManager();
    ~VirtualAdapterManager();
    
    // Gateway management
    auto set_real_adapter_as_gateway(const std::string& adapter_name) -> std::expected<void, std::string>;
    auto get_gateway() -> NetworkGateway*;
    
    // Virtual adapter management
    auto create_virtual_adapter(const VirtualAdapterConfig& config) -> std::expected<std::string, std::string>;
    auto delete_virtual_adapter(const std::string& adapter_id) -> bool;
    auto get_virtual_adapter(const std::string& adapter_id) -> VirtualAdapter*;
    auto list_virtual_adapters() const -> std::vector<std::string>;
    
    // Hub management
    auto create_hub(const std::string& name) -> std::expected<std::string, std::string>;
    auto delete_hub(const std::string& hub_id) -> bool;
    auto connect_adapter_to_hub(const std::string& adapter_id, const std::string& hub_id) -> bool;
    auto disconnect_adapter_from_hub(const std::string& adapter_id, const std::string& hub_id) -> bool;
    auto get_hub(const std::string& hub_id) -> VirtualHub*;
    auto list_hubs() const -> std::vector<std::string>;
    
    // VPC management
    auto create_vpc(const std::string& vpc_id, const ipv4_address& base_address, int prefix_length = 24) -> std::expected<void, std::string>;
    auto add_adapter_to_vpc(const std::string& adapter_id, const std::string& vpc_id) -> bool;
    auto remove_adapter_from_vpc(const std::string& adapter_id, const std::string& vpc_id) -> bool;
    auto get_vpc_adapters(const std::string& vpc_id) const -> std::vector<std::string>;
    
    // Network interface enumeration
    auto enumerate_real_adapters() -> std::vector<NetworkInterface>;
    auto get_adapter_info(const std::string& adapter_id) -> std::optional<NetworkInterface>;
    
    // Dual-stack validation and linking
    auto validate_dual_stack(const std::string& adapter_id) -> std::expected<void, std::string>;
    auto link_ipv4_ipv6(const std::string& adapter_id, const ipv4_address& ipv4, const ipv6_address& ipv6) -> bool;
    
    // DNS management
    auto configure_dns(const std::string& adapter_id, const std::vector<DNSServer>& dns_servers) -> bool;
    auto add_google_dns(const std::string& adapter_id) -> bool;
    
    // Statistics
    auto get_adapter_statistics(const std::string& adapter_id) -> std::optional<NetworkInterface>;
};

// Virtual Adapter - Individual virtual network adapter
class VirtualAdapter {
private:
    std::string adapter_id_;
    VirtualAdapterConfig config_;
    AdapterState state_;
    mutable std::mutex adapter_mutex_;
    
    // Connected hub
    std::optional<std::string> connected_hub_id_;
    
    // Statistics
    std::atomic<std::uint64_t> bytes_sent_;
    std::atomic<std::uint64_t> bytes_received_;
    std::atomic<std::uint64_t> packets_sent_;
    std::atomic<std::uint64_t> packets_received_;
    
    // IPv4/IPv6 linking
    std::map<ipv4_address, ipv6_address> ipv4_to_ipv6_map_;
    std::map<ipv6_address, ipv4_address> ipv6_to_ipv4_map_;
    
public:
    VirtualAdapter(const std::string& adapter_id, const VirtualAdapterConfig& config);
    ~VirtualAdapter();
    
    // Adapter lifecycle
    auto enable() -> std::expected<void, std::string>;
    auto disable() -> void;
    auto get_state() const -> AdapterState { return state_; }
    
    // Configuration
    auto get_config() const -> const VirtualAdapterConfig& { return config_; }
    auto update_config(const VirtualAdapterConfig& config) -> std::expected<void, std::string>;
    
    // Hub connection
    auto connect_to_hub(const std::string& hub_id) -> bool;
    auto disconnect_from_hub() -> void;
    auto get_connected_hub() const -> std::optional<std::string> { return connected_hub_id_; }
    
    // IPv4/IPv6 linking
    auto link_addresses(const ipv4_address& ipv4, const ipv6_address& ipv6) -> void;
    auto get_ipv6_for_ipv4(const ipv4_address& ipv4) -> std::optional<ipv6_address>;
    auto get_ipv4_for_ipv6(const ipv6_address& ipv6) -> std::optional<ipv4_address>;
    
    // Statistics
    auto get_statistics() const -> NetworkInterface;
    
    // Adapter info
    auto get_adapter_id() const -> const std::string& { return adapter_id_; }
};

} // namespace network
} // namespace dualstack