/**
 * Amphisbaena 🐍 - Virtual Network Adapter System Implementation
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * ENTERPRISE-GRADE IMPLEMENTATION
 */

// Include format header fix BEFORE any standard headers
#include "../../include/dualstack_net26/fix_format_header.h"
#include "../../include/dualstack_net26/network/virtual_adapter.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>  // For sockaddr_in and sockaddr_in6
#ifdef __linux__
#include <linux/if_link.h>
#endif
#include <unistd.h>
#endif

namespace dualstack {
namespace network {

// ============================================================================
// VirtualHub Implementation
// ============================================================================

VirtualHub::VirtualHub(const std::string& hub_id, const std::string& name)
    : hub_id_(hub_id), name_(name) {
}

VirtualHub::~VirtualHub() = default;

auto VirtualHub::add_adapter(const std::string& adapter_id) -> bool {
    std::lock_guard<std::mutex> lock(hub_mutex_);
    if (std::find(connected_adapter_ids_.begin(), connected_adapter_ids_.end(), adapter_id) 
        != connected_adapter_ids_.end()) {
        return false;  // Already connected
    }
    connected_adapter_ids_.push_back(adapter_id);
    return true;
}

auto VirtualHub::remove_adapter(const std::string& adapter_id) -> bool {
    std::lock_guard<std::mutex> lock(hub_mutex_);
    auto it = std::find(connected_adapter_ids_.begin(), connected_adapter_ids_.end(), adapter_id);
    if (it == connected_adapter_ids_.end()) {
        return false;
    }
    connected_adapter_ids_.erase(it);
    return true;
}

auto VirtualHub::get_connected_adapters() const -> std::vector<std::string> {
    std::lock_guard<std::mutex> lock(hub_mutex_);
    return connected_adapter_ids_;
}

auto VirtualHub::route_ipv4(const ipv4_address& dest) -> std::optional<std::string> {
    std::lock_guard<std::mutex> lock(hub_mutex_);
    // Find longest prefix match
    std::optional<std::string> best_match;
    std::uint32_t best_prefix = 0;
    
    for (const auto& [addr, adapter_id] : ipv4_routing_table_) {
        // Calculate matching prefix length
        std::uint32_t addr_val = addr.address;
        std::uint32_t dest_val = dest.address;
        std::uint32_t mask = 0xFFFFFFFF;
        int prefix = 0;
        
        for (int i = 0; i < 32; ++i) {
            if ((addr_val & mask) == (dest_val & mask)) {
                prefix = 32 - i;
                break;
            }
            mask <<= 1;
        }
        
        if (prefix > static_cast<int>(best_prefix)) {
            best_prefix = prefix;
            best_match = adapter_id;
        }
    }
    
    return best_match;
}

auto VirtualHub::route_ipv6(const ipv6_address& dest) -> std::optional<std::string> {
    std::lock_guard<std::mutex> lock(hub_mutex_);
    // Find longest prefix match for IPv6
    std::optional<std::string> best_match;
    int best_prefix = 0;
    
    for (const auto& [addr, adapter_id] : ipv6_routing_table_) {
        // Calculate matching prefix length
        int prefix = 0;
        if (addr.high == dest.high) {
            if (addr.low == dest.low) {
                prefix = 128;
            } else {
                // Calculate prefix match in low 64 bits
                std::uint64_t diff = addr.low ^ dest.low;
                prefix = 64;
                while (diff != 0 && prefix < 128) {
                    diff >>= 1;
                    prefix++;
                }
            }
        } else {
            // Calculate prefix match in high 64 bits
            std::uint64_t diff = addr.high ^ dest.high;
            prefix = 0;
            while (diff != 0 && prefix < 64) {
                diff >>= 1;
                prefix++;
            }
        }
        
        if (prefix > best_prefix) {
            best_prefix = prefix;
            best_match = adapter_id;
        }
    }
    
    return best_match;
}

auto VirtualHub::add_route_ipv4(const ipv4_address& dest, const std::string& adapter_id) -> void {
    std::lock_guard<std::mutex> lock(hub_mutex_);
    ipv4_routing_table_[dest] = adapter_id;
}

auto VirtualHub::add_route_ipv6(const ipv6_address& dest, const std::string& adapter_id) -> void {
    std::lock_guard<std::mutex> lock(hub_mutex_);
    ipv6_routing_table_[dest] = adapter_id;
}

// ============================================================================
// NetworkGateway Implementation
// ============================================================================

NetworkGateway::NetworkGateway(const std::string& real_adapter_name)
    : gateway_id_("gateway_" + real_adapter_name)
    , real_adapter_name_(real_adapter_name)
    , use_google_dns_(true) {
    
    // Add Google DNS by default
    DNSServer google_dns_v4;
    auto google_ipv4 = IPAddress::from_string("8.8.8.8");
    if (google_ipv4.has_value() && google_ipv4->is_ipv4()) {
        google_dns_v4.address = google_ipv4.value();
        google_dns_v4.name = "Google DNS IPv4";
        google_dns_v4.is_ipv6 = false;
        google_dns_v4.priority = 1;
        dns_servers_.push_back(google_dns_v4);
    }
    
    DNSServer google_dns_v6;
    auto google_ipv6 = IPAddress::from_string("2001:4860:4860::8888");
    if (google_ipv6.has_value() && google_ipv6->is_ipv6()) {
        google_dns_v6.address = google_ipv6.value();
        google_dns_v6.name = "Google DNS IPv6";
        google_dns_v6.is_ipv6 = true;
        google_dns_v6.priority = 1;
        dns_servers_.push_back(google_dns_v6);
    }
}

NetworkGateway::~NetworkGateway() {
    shutdown();
}

auto NetworkGateway::initialize() -> std::expected<void, std::string> {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    
    // Enumerate real adapters and find the specified one
    auto adapters = VirtualAdapterManager{}.enumerate_real_adapters();
    auto it = std::find_if(adapters.begin(), adapters.end(),
        [this](const NetworkInterface& iface) {
            return iface.name == real_adapter_name_;
        });
    
    if (it == adapters.end()) {
        return std::unexpected(std::string("Real adapter not found: " + real_adapter_name_));
    }
    
    real_adapter_info_ = *it;
    return {};
}

auto NetworkGateway::shutdown() -> void {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    virtual_adapter_ids_.clear();
    ipv4_gateway_routes_.clear();
    ipv6_gateway_routes_.clear();
    nat_ipv4_table_.clear();
}

auto NetworkGateway::is_initialized() const -> bool {
    return !real_adapter_info_.name.empty();
}

auto NetworkGateway::get_real_adapter_info() const -> NetworkInterface {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    return real_adapter_info_;
}

auto NetworkGateway::set_as_gateway() -> std::expected<void, std::string> {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    
    if (real_adapter_info_.name.empty()) {
        return std::unexpected(std::string("Gateway not initialized"));
    }
    
    // Set up gateway routes
    if (real_adapter_info_.ipv4_gateway.has_value()) {
        ipv4_gateway_routes_[ipv4_address(0)] = real_adapter_name_;  // Default route
    }
    
    if (real_adapter_info_.ipv6_gateway.has_value()) {
        ipv6_gateway_routes_[ipv6_address(0, 0)] = real_adapter_name_;  // Default route
    }
    
    return {};
}

auto NetworkGateway::register_virtual_adapter(const std::string& adapter_id) -> bool {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    if (std::find(virtual_adapter_ids_.begin(), virtual_adapter_ids_.end(), adapter_id) 
        != virtual_adapter_ids_.end()) {
        return false;
    }
    virtual_adapter_ids_.push_back(adapter_id);
    return true;
}

auto NetworkGateway::unregister_virtual_adapter(const std::string& adapter_id) -> bool {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    auto it = std::find(virtual_adapter_ids_.begin(), virtual_adapter_ids_.end(), adapter_id);
    if (it == virtual_adapter_ids_.end()) {
        return false;
    }
    virtual_adapter_ids_.erase(it);
    return true;
}

auto NetworkGateway::route_packet_ipv4(const ipv4_address& dest [[maybe_unused]], const ipv4_address& src [[maybe_unused]]) -> std::optional<std::string> {
    std::lock_guard<std::mutex> lock(gateway_mutex_);

    // Check if destination is in virtual adapter network
    for (const auto& adapter_id [[maybe_unused]] : virtual_adapter_ids_) {
        // For PsiForceDB web app, route local traffic to virtual adapters
        // External traffic goes through real adapter with Google DNS
    }

    // Default route through real adapter (uses Google DNS)
    return real_adapter_name_;
}

auto NetworkGateway::route_packet_ipv6(const ipv6_address& dest [[maybe_unused]], const ipv6_address& src [[maybe_unused]]) -> std::optional<std::string> {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    
    // Check if destination is in virtual adapter network
    for (const auto& adapter_id [[maybe_unused]] : virtual_adapter_ids_) {
        // TODO: Check if dest is in adapter's network range
    }
    
    // Default route through real adapter
    return real_adapter_name_;
}

auto NetworkGateway::translate_nat_ipv4(const ipv4_address& private_ip) -> std::optional<ipv4_address> {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    
    auto it = nat_ipv4_table_.find(private_ip);
    if (it != nat_ipv4_table_.end()) {
        return it->second;
    }
    
    // Allocate new NAT translation (use real adapter's IP)
    if (!real_adapter_info_.ipv4_addresses.empty()) {
        ipv4_address public_ip = real_adapter_info_.ipv4_addresses[0];
        nat_ipv4_table_[private_ip] = public_ip;
        return public_ip;
    }
    
    return std::nullopt;
}

auto NetworkGateway::add_dns_server(const DNSServer& server) -> void {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    dns_servers_.push_back(server);
    std::sort(dns_servers_.begin(), dns_servers_.end(),
        [](const DNSServer& a, const DNSServer& b) {
            return a.priority < b.priority;
        });
}

auto NetworkGateway::set_google_dns(bool enable) -> void {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    use_google_dns_ = enable;
    
    if (enable) {
        // Add Google DNS if not already present
        bool has_google = false;
        for (const auto& dns : dns_servers_) {
            if (dns.name.find("Google") != std::string::npos) {
                has_google = true;
                break;
            }
        }
        
        if (!has_google) {
            DNSServer google_dns_v4;
            auto google_ipv4 = IPAddress::from_string("8.8.8.8");
            if (google_ipv4.has_value() && google_ipv4->is_ipv4()) {
                google_dns_v4.address = google_ipv4.value();
                google_dns_v4.name = "Google DNS IPv4";
                google_dns_v4.is_ipv6 = false;
                google_dns_v4.priority = 1;
                dns_servers_.push_back(google_dns_v4);
            }
            
            DNSServer google_dns_v6;
            auto google_ipv6 = IPAddress::from_string("2001:4860:4860::8888");
            if (google_ipv6.has_value() && google_ipv6->is_ipv6()) {
                google_dns_v6.address = google_ipv6.value();
                google_dns_v6.name = "Google DNS IPv6";
                google_dns_v6.is_ipv6 = true;
                google_dns_v6.priority = 1;
                dns_servers_.push_back(google_dns_v6);
            }
        }
    }
}

auto NetworkGateway::resolve_dns(const std::string& hostname [[maybe_unused]], bool prefer_ipv6 [[maybe_unused]]) -> std::expected<IPAddress, std::string> {
    std::lock_guard<std::mutex> lock(gateway_mutex_);
    
    // TODO: Implement actual DNS resolution
    // For now, return error
    return std::unexpected<std::string>("DNS resolution not yet implemented");
}

// ============================================================================
// VirtualAdapter Implementation
// ============================================================================

VirtualAdapter::VirtualAdapter(const std::string& adapter_id, const VirtualAdapterConfig& config)
    : adapter_id_(adapter_id)
    , config_(config)
    , state_(AdapterState::DISABLED)
    , bytes_sent_(0)
    , bytes_received_(0)
    , packets_sent_(0)
    , packets_received_(0) {
}

VirtualAdapter::~VirtualAdapter() = default;

auto VirtualAdapter::enable() -> std::expected<void, std::string> {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    
    if (state_ == AdapterState::ENABLED || state_ == AdapterState::CONNECTED) {
        return {};  // Already enabled
    }
    
    state_ = AdapterState::CONNECTING;
    
    // Validate configuration
    if (config_.type == AdapterType::VIRTUAL) {
        if (!config_.ipv4_addr.has_value() && !config_.ipv6_addr.has_value()) {
            state_ = AdapterState::ERROR;
            return std::unexpected(std::string("No IP address configured"));
        }
    }
    
    state_ = AdapterState::ENABLED;
    return {};
}

auto VirtualAdapter::disable() -> void {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    state_ = AdapterState::DISABLED;
}

auto VirtualAdapter::update_config(const VirtualAdapterConfig& config) -> std::expected<void, std::string> {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    
    if (state_ == AdapterState::CONNECTED || state_ == AdapterState::CONNECTING) {
        return std::unexpected(std::string("Cannot update config while adapter is active"));
    }
    
    config_ = config;
    return {};
}

auto VirtualAdapter::connect_to_hub(const std::string& hub_id) -> bool {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    if (connected_hub_id_.has_value()) {
        return false;  // Already connected to a hub
    }
    connected_hub_id_ = hub_id;
    return true;
}

auto VirtualAdapter::disconnect_from_hub() -> void {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    connected_hub_id_ = std::nullopt;
}

auto VirtualAdapter::link_addresses(const ipv4_address& ipv4, const ipv6_address& ipv6) -> void {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    ipv4_to_ipv6_map_[ipv4] = ipv6;
    ipv6_to_ipv4_map_[ipv6] = ipv4;
}

auto VirtualAdapter::get_ipv6_for_ipv4(const ipv4_address& ipv4) -> std::optional<ipv6_address> {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    auto it = ipv4_to_ipv6_map_.find(ipv4);
    if (it != ipv4_to_ipv6_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto VirtualAdapter::get_ipv4_for_ipv6(const ipv6_address& ipv6) -> std::optional<ipv4_address> {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    auto it = ipv6_to_ipv4_map_.find(ipv6);
    if (it != ipv6_to_ipv4_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto VirtualAdapter::get_statistics() const -> NetworkInterface {
    std::lock_guard<std::mutex> lock(adapter_mutex_);
    
    NetworkInterface info;
    info.name = config_.name;
    info.description = config_.description;
    info.type = config_.type;
    info.state = state_;
    info.is_physical = false;
    
    if (config_.ipv4_addr.has_value()) {
        info.ipv4_addresses.push_back(config_.ipv4_addr.value());
    }
    if (config_.ipv6_addr.has_value()) {
        info.ipv6_addresses.push_back(config_.ipv6_addr.value());
    }
    
    info.bytes_sent = bytes_sent_.load();
    info.bytes_received = bytes_received_.load();
    info.packets_sent = packets_sent_.load();
    info.packets_received = packets_received_.load();
    
    return info;
}

// ============================================================================
// VirtualAdapterManager Implementation
// ============================================================================

VirtualAdapterManager::VirtualAdapterManager()
    : adapter_counter_(0) {
}

VirtualAdapterManager::~VirtualAdapterManager() = default;

auto VirtualAdapterManager::set_real_adapter_as_gateway(const std::string& adapter_name) -> std::expected<void, std::string> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    gateway_ = std::make_unique<NetworkGateway>(adapter_name);
    auto result = gateway_->initialize();
    if (!result.has_value()) {
        gateway_.reset();
        return result;
    }
    
    result = gateway_->set_as_gateway();
    if (!result.has_value()) {
        gateway_.reset();
        return result;
    }
    
    return {};
}

auto VirtualAdapterManager::get_gateway() -> NetworkGateway* {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return gateway_.get();
}

auto VirtualAdapterManager::create_virtual_adapter(const VirtualAdapterConfig& config) -> std::expected<std::string, std::string> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    std::string adapter_id = "vadapter_" + std::to_string(adapter_counter_.fetch_add(1));
    
    auto adapter = std::make_unique<VirtualAdapter>(adapter_id, config);
    auto enable_result = adapter->enable();
    if (!enable_result.has_value()) {
        return std::unexpected(std::string(enable_result.error()));
    }
    
    adapters_[adapter_id] = std::move(adapter);
    
    // Register with gateway if available
    if (gateway_) {
        gateway_->register_virtual_adapter(adapter_id);
    }
    
    return adapter_id;
}

auto VirtualAdapterManager::delete_virtual_adapter(const std::string& adapter_id) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        return false;
    }
    
    // Unregister from gateway
    if (gateway_) {
        gateway_->unregister_virtual_adapter(adapter_id);
    }
    
    adapters_.erase(it);
    return true;
}

auto VirtualAdapterManager::get_virtual_adapter(const std::string& adapter_id) -> VirtualAdapter* {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        return nullptr;
    }
    return it->second.get();
}

auto VirtualAdapterManager::list_virtual_adapters() const -> std::vector<std::string> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    std::vector<std::string> ids;
    ids.reserve(adapters_.size());
    for (const auto& [id, adapter] : adapters_) {
        ids.push_back(id);
    }
    return ids;
}

auto VirtualAdapterManager::create_hub(const std::string& name) -> std::expected<std::string, std::string> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    std::string hub_id = "hub_" + std::to_string(hubs_.size() + 1);
    auto hub = std::make_unique<VirtualHub>(hub_id, name);
    hubs_[hub_id] = std::move(hub);
    
    return hub_id;
}

auto VirtualAdapterManager::delete_hub(const std::string& hub_id) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return hubs_.erase(hub_id) > 0;
}

auto VirtualAdapterManager::connect_adapter_to_hub(const std::string& adapter_id, const std::string& hub_id) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto adapter_it = adapters_.find(adapter_id);
    auto hub_it = hubs_.find(hub_id);
    
    if (adapter_it == adapters_.end() || hub_it == hubs_.end()) {
        return false;
    }
    
    if (!adapter_it->second->connect_to_hub(hub_id)) {
        return false;
    }
    
    return hub_it->second->add_adapter(adapter_id);
}

auto VirtualAdapterManager::disconnect_adapter_from_hub(const std::string& adapter_id, const std::string& hub_id) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto adapter_it = adapters_.find(adapter_id);
    auto hub_it = hubs_.find(hub_id);
    
    if (adapter_it == adapters_.end() || hub_it == hubs_.end()) {
        return false;
    }
    
    adapter_it->second->disconnect_from_hub();
    return hub_it->second->remove_adapter(adapter_id);
}

auto VirtualAdapterManager::get_hub(const std::string& hub_id) -> VirtualHub* {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = hubs_.find(hub_id);
    if (it == hubs_.end()) {
        return nullptr;
    }
    return it->second.get();
}

auto VirtualAdapterManager::list_hubs() const -> std::vector<std::string> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    std::vector<std::string> ids;
    ids.reserve(hubs_.size());
    for (const auto& [id, hub] : hubs_) {
        ids.push_back(id);
    }
    return ids;
}

auto VirtualAdapterManager::create_vpc(const std::string& vpc_id, const ipv4_address& base_address [[maybe_unused]], int prefix_length [[maybe_unused]]) -> std::expected<void, std::string> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    if (vpc_adapters_.find(vpc_id) != vpc_adapters_.end()) {
        return std::unexpected(std::string("VPC already exists: " + vpc_id));
    }
    
    vpc_adapters_[vpc_id] = std::vector<std::string>();
    return {};
}

auto VirtualAdapterManager::add_adapter_to_vpc(const std::string& adapter_id, const std::string& vpc_id) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto vpc_it = vpc_adapters_.find(vpc_id);
    if (vpc_it == vpc_adapters_.end()) {
        return false;
    }
    
    auto& adapters = vpc_it->second;
    if (std::find(adapters.begin(), adapters.end(), adapter_id) != adapters.end()) {
        return false;  // Already in VPC
    }
    
    adapters.push_back(adapter_id);
    return true;
}

auto VirtualAdapterManager::remove_adapter_from_vpc(const std::string& adapter_id, const std::string& vpc_id) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto vpc_it = vpc_adapters_.find(vpc_id);
    if (vpc_it == vpc_adapters_.end()) {
        return false;
    }
    
    auto& adapters = vpc_it->second;
    auto it = std::find(adapters.begin(), adapters.end(), adapter_id);
    if (it == adapters.end()) {
        return false;
    }
    
    adapters.erase(it);
    return true;
}

auto VirtualAdapterManager::get_vpc_adapters(const std::string& vpc_id) const -> std::vector<std::string> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = vpc_adapters_.find(vpc_id);
    if (it == vpc_adapters_.end()) {
        return {};
    }
    return it->second;
}

auto VirtualAdapterManager::enumerate_real_adapters() -> std::vector<NetworkInterface> {
    std::vector<NetworkInterface> interfaces;
    
#ifdef _WIN32
    // Windows implementation using GetAdaptersAddresses
    ULONG buffer_size = 0;
    GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &buffer_size);
    
    std::vector<BYTE> buffer(buffer_size);
    PIP_ADAPTER_ADDRESSES adapters = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    
    if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, adapters, &buffer_size) == ERROR_SUCCESS) {
        for (PIP_ADAPTER_ADDRESSES adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
            NetworkInterface iface;
            iface.name = std::string(adapter->AdapterName);
            // Convert wide string to narrow string
            if (adapter->Description) {
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, adapter->Description, -1, nullptr, 0, nullptr, nullptr);
                if (size_needed > 0) {
                    std::vector<char> buffer(size_needed);
                    WideCharToMultiByte(CP_UTF8, 0, adapter->Description, -1, buffer.data(), size_needed, nullptr, nullptr);
                    iface.description = std::string(buffer.data());
                } else {
                    iface.description = "";
                }
            } else {
                iface.description = "";
            }
            iface.type = AdapterType::REAL;
            iface.state = adapter->OperStatus == IfOperStatusUp ? AdapterState::CONNECTED : AdapterState::DISABLED;
            iface.is_physical = true;
            
            // Get MAC address
            std::stringstream mac_ss;
            for (DWORD i = 0; i < adapter->PhysicalAddressLength; ++i) {
                if (i > 0) mac_ss << ":";
                mac_ss << std::hex << std::setfill('0') << std::setw(2) 
                       << static_cast<int>(adapter->PhysicalAddress[i]);
            }
            iface.mac_address = mac_ss.str();
            
            // Get IP addresses
            for (PIP_ADAPTER_UNICAST_ADDRESS addr = adapter->FirstUnicastAddress; 
                 addr != nullptr; addr = addr->Next) {
                if (addr->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(addr->Address.lpSockaddr);
                    ipv4_address ipv4(ntohl(sin->sin_addr.s_addr));
                    iface.ipv4_addresses.push_back(ipv4);
                } else if (addr->Address.lpSockaddr->sa_family == AF_INET6) {
                    sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(addr->Address.lpSockaddr);
                    // Convert IPv6 address
                    std::uint64_t high = 0, low = 0;
                    for (int i = 0; i < 8; ++i) {
                        high |= (static_cast<std::uint64_t>(sin6->sin6_addr.s6_addr[i]) << (56 - i * 8));
                    }
                    for (int i = 0; i < 8; ++i) {
                        low |= (static_cast<std::uint64_t>(sin6->sin6_addr.s6_addr[i + 8]) << (56 - i * 8));
                    }
                    ipv6_address ipv6(high, low);
                    iface.ipv6_addresses.push_back(ipv6);
                }
            }
            
            interfaces.push_back(iface);
        }
    }
#else
    // Linux implementation using getifaddrs
    struct ifaddrs* ifaddrs_list = nullptr;
    if (getifaddrs(&ifaddrs_list) == 0) {
        std::map<std::string, NetworkInterface*> interface_map;
        
        for (struct ifaddrs* ifa = ifaddrs_list; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) continue;
            
            std::string name = ifa->ifa_name;
            NetworkInterface* iface = nullptr;
            
            auto it = interface_map.find(name);
            if (it == interface_map.end()) {
                NetworkInterface new_iface;
                new_iface.name = name;
                new_iface.description = name;
                new_iface.type = AdapterType::REAL;
                new_iface.state = (ifa->ifa_flags & IFF_UP) ? AdapterState::CONNECTED : AdapterState::DISABLED;
                new_iface.is_physical = !(ifa->ifa_flags & IFF_LOOPBACK);
                
                interfaces.push_back(new_iface);
                iface = &interfaces.back();
                interface_map[name] = iface;
            } else {
                iface = it->second;
            }
            
            if (ifa->ifa_addr->sa_family == AF_INET) {
                sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
                ipv4_address ipv4(ntohl(sin->sin_addr.s_addr));
                iface->ipv4_addresses.push_back(ipv4);
                
                // Check if this is the gateway
                if (ifa->ifa_flags & IFF_UP && !iface->ipv4_gateway.has_value()) {
                    // Try to determine gateway (simplified)
                }
            } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr);
                std::uint64_t high = 0, low = 0;
                for (int i = 0; i < 8; ++i) {
                    high |= (static_cast<std::uint64_t>(sin6->sin6_addr.s6_addr[i]) << (56 - i * 8));
                }
                for (int i = 0; i < 8; ++i) {
                    low |= (static_cast<std::uint64_t>(sin6->sin6_addr.s6_addr[i + 8]) << (56 - i * 8));
                }
                ipv6_address ipv6(high, low);
                iface->ipv6_addresses.push_back(ipv6);
            }
        }
        
        freeifaddrs(ifaddrs_list);
    }
#endif
    
    return interfaces;
}

auto VirtualAdapterManager::get_adapter_info(const std::string& adapter_id) -> std::optional<NetworkInterface> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        return std::nullopt;
    }
    
    return it->second->get_statistics();
}

auto VirtualAdapterManager::validate_dual_stack(const std::string& adapter_id) -> std::expected<void, std::string> {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        return std::unexpected(std::string("Adapter not found: " + adapter_id));
    }
    
    const auto& config = it->second->get_config();
    
    // Validate both IPv4 and IPv6 are configured
    if (!config.ipv4_addr.has_value()) {
        return std::unexpected(std::string("IPv4 address not configured"));
    }
    
    if (!config.ipv6_addr.has_value()) {
        return std::unexpected(std::string("IPv6 address not configured"));
    }
    
    // Link them together
    it->second->link_addresses(config.ipv4_addr.value(), config.ipv6_addr.value());
    
    return {};
}

auto VirtualAdapterManager::link_ipv4_ipv6(const std::string& adapter_id, const ipv4_address& ipv4, const ipv6_address& ipv6) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        return false;
    }
    
    it->second->link_addresses(ipv4, ipv6);
    return true;
}

auto VirtualAdapterManager::configure_dns(const std::string& adapter_id, const std::vector<DNSServer>& dns_servers [[maybe_unused]]) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        return false;
    }
    
    // TODO: Update adapter's DNS configuration
    return true;
}

auto VirtualAdapterManager::add_google_dns(const std::string& adapter_id) -> bool {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    DNSServer google_dns_v4;
    auto google_ipv4 = IPAddress::from_string("8.8.8.8");
    if (google_ipv4.has_value() && google_ipv4->is_ipv4()) {
        google_dns_v4.address = google_ipv4.value();
        google_dns_v4.name = "Google DNS IPv4";
        google_dns_v4.is_ipv6 = false;
        google_dns_v4.priority = 1;
    }
    
    DNSServer google_dns_v6;
    auto google_ipv6 = IPAddress::from_string("2001:4860:4860::8888");
    if (google_ipv6.has_value() && google_ipv6->is_ipv6()) {
        google_dns_v6.address = google_ipv6.value();
        google_dns_v6.name = "Google DNS IPv6";
        google_dns_v6.is_ipv6 = true;
        google_dns_v6.priority = 1;
    }
    
    return configure_dns(adapter_id, {google_dns_v4, google_dns_v6});
}

auto VirtualAdapterManager::get_adapter_statistics(const std::string& adapter_id) -> std::optional<NetworkInterface> {
    return get_adapter_info(adapter_id);
}

} // namespace network
} // namespace dualstack