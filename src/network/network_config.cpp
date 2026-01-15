/**
 * Amphisbaena 🐍 - Enhanced Network Configuration System Implementation
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * Implementation of easy-to-use network configuration system
 */

// Include format header fix BEFORE any standard headers
#include "../../include/dualstack_net26/fix_format_header.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <regex>
#include <mutex>
#include <functional>
#include "../../include/dualstack_net26/network/network_config.h"
#include "../core/ip_address.h"

using ::make_unexpected_value;

namespace dualstack {
namespace network {
namespace config {

// ============================================================================
// SubnetConfig Implementation
// ============================================================================

SubnetConfig::SubnetConfig()
    : prefix_length(0)
    , type(NetworkType::PRIVATE)
    , is_ipv6(false)
    , allow_inbound(true)
    , allow_outbound(true)
    , require_encryption(false)
    , require_authentication(false)
    , vnc_port(5900)
    , vnc_encrypted(false)
    , created_at(std::chrono::system_clock::now())
    , updated_at(std::chrono::system_clock::now())
{
}

SubnetConfig::SubnetConfig(const std::string& name, const IPAddress& addr, uint8_t prefix, NetworkType type)
    : name(name)
    , network_address(addr)
    , prefix_length(prefix)
    , type(type)
    , is_ipv6(addr.is_ipv6())
    , allow_inbound(true)
    , allow_outbound(true)
    , require_encryption(type == NetworkType::VPN || type == NetworkType::VPC)
    , require_authentication(type == NetworkType::VPN || type == NetworkType::VPC)
    , vnc_port(5900)
    , vnc_encrypted(false)
    , created_at(std::chrono::system_clock::now())
    , updated_at(std::chrono::system_clock::now())
{
}

bool SubnetConfig::contains(const IPAddress& addr) const {
    // Simple implementation - check if address is in subnet range
    // For production, implement proper CIDR matching
    if (is_ipv6 != addr.is_ipv6()) {
        return false;
    }
    
    // TODO: Implement proper CIDR matching
    // For now, return true if same address family
    return true;
}

std::string SubnetConfig::to_cidr() const {
    std::ostringstream oss;
    oss << network_address.to_string() << "/" << static_cast<int>(prefix_length);
    return oss.str();
}

std::expected<SubnetConfig, std::string> SubnetConfig::from_cidr(const std::string& cidr, NetworkType type) {
    // Parse CIDR notation: "192.168.1.0/24" or "2001:db8::/32"
    std::regex cidr_regex(R"(([^/]+)/(\d+))");
    std::smatch match;
    
    if (!std::regex_match(cidr, match, cidr_regex)) {
#if __cpp_lib_expected >= 202202L
        return std::unexpected(std::string("Invalid CIDR format"));
#else
        return std::make_unexpected(std::string("Invalid CIDR format"));
#endif
    }
    
    std::string addr_str = match[1].str();
    int prefix = std::stoi(match[2].str());
    
    if (prefix < 0 || prefix > 128) {
        return make_unexpected_value(std::string("Invalid prefix length"));
    }
    
    auto ip_result = IPAddress::from_string(addr_str);
    if (!ip_result.has_value()) {
        return make_unexpected_value(std::string("Invalid IP address"));
    }
    
    IPAddress addr = *ip_result;
    bool is_ipv6 = addr.is_ipv6();
    
    if (is_ipv6 && prefix > 128) {
        return make_unexpected_value(std::string("IPv6 prefix length cannot exceed 128"));
    }
    if (!is_ipv6 && prefix > 32) {
        return make_unexpected_value(std::string("IPv4 prefix length cannot exceed 32"));
    }
    
    SubnetConfig config;
    config.network_address = addr;
    config.prefix_length = static_cast<uint8_t>(prefix);
    config.type = type;
    config.is_ipv6 = is_ipv6;
    config.name = cidr;
    config.created_at = std::chrono::system_clock::now();
    config.updated_at = std::chrono::system_clock::now();
    
    return config;
}

// ============================================================================
// InterfaceConfig Implementation
// ============================================================================

InterfaceConfig::InterfaceConfig()
    : is_up(false)
    , is_loopback(false)
    , primary_type(NetworkType::PRIVATE)
    , mtu(1500)
    , speed_mbps(1000)
    , promiscuous_mode(false)
{
}

// ============================================================================
// RouteConfig Implementation
// ============================================================================

RouteConfig::RouteConfig()
    : destination_prefix(0)
    , metric(100)
    , is_default(false)
{
}

// ============================================================================
// NetworkProfile Implementation
// ============================================================================

NetworkProfile::NetworkProfile()
    : firewall_enabled(true)
    , nat_enabled(false)
    , ip_forwarding_enabled(false)
    , created_at(std::chrono::system_clock::now())
    , updated_at(std::chrono::system_clock::now())
{
}

NetworkProfile::NetworkProfile(const std::string& name)
    : profile_name(name)
    , firewall_enabled(true)
    , nat_enabled(false)
    , ip_forwarding_enabled(false)
    , created_at(std::chrono::system_clock::now())
    , updated_at(std::chrono::system_clock::now())
{
}

// ============================================================================
// NetworkConfigEditor Implementation
// ============================================================================

NetworkConfigEditor::NetworkConfigEditor() {
    current_profile_ = std::make_unique<NetworkProfile>("default");
}

NetworkConfigEditor::~NetworkConfigEditor() = default;

bool NetworkConfigEditor::add_subnet(const std::string& cidr, NetworkType type, const std::string& name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto result = SubnetConfig::from_cidr(cidr, type);
    if (!result.has_value()) {
        return false;
    }
    
    SubnetConfig config = result.value();
    if (!name.empty()) {
        config.name = name;
    } else {
        config.name = generate_subnet_name(cidr, type);
    }
    
    if (!validate_subnet(config)) {
        return false;
    }
    
    std::string cidr_key = config.to_cidr();
    subnets_by_cidr_[cidr_key] = config;
    
    if (current_profile_) {
        current_profile_->subnets.push_back(config);
    }
    
    update_subnet_indexes();
    return true;
}

bool NetworkConfigEditor::remove_subnet(const std::string& cidr) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = subnets_by_cidr_.find(cidr);
    if (it == subnets_by_cidr_.end()) {
        return false;
    }
    
    [[maybe_unused]] NetworkType type = it->second.type;
    subnets_by_cidr_.erase(it);
    
    if (current_profile_) {
        auto& subnets = current_profile_->subnets;
        subnets.erase(
            std::remove_if(subnets.begin(), subnets.end(),
                [&cidr](const SubnetConfig& s) { return s.to_cidr() == cidr; }),
            subnets.end()
        );
    }
    
    update_subnet_indexes();
    return true;
}

std::optional<SubnetConfig> NetworkConfigEditor::get_subnet_for(const IPAddress& addr) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    // Find the most specific subnet that contains the address
    SubnetConfig* best_match = nullptr;
    uint8_t best_prefix = 0;
    
    for (const auto& pair : subnets_by_cidr_) {
        if (pair.second.contains(addr)) {
            if (pair.second.prefix_length > best_prefix) {
                best_prefix = pair.second.prefix_length;
                best_match = const_cast<SubnetConfig*>(&pair.second);
            }
        }
    }
    
    if (best_match) {
        return *best_match;
    }
    
    return {};
}

bool NetworkConfigEditor::is_in_network_type(const IPAddress& addr, NetworkType type) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = subnets_by_type_.find(type);
    if (it == subnets_by_type_.end()) {
        return false;
    }
    
    for (const auto& cidr : it->second) {
        auto subnet_it = subnets_by_cidr_.find(cidr);
        if (subnet_it != subnets_by_cidr_.end()) {
            if (subnet_it->second.contains(addr)) {
                return true;
            }
        }
    }
    
    return false;
}

std::vector<SubnetConfig> NetworkConfigEditor::get_subnets_by_type(NetworkType type) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::vector<SubnetConfig> result;
    auto it = subnets_by_type_.find(type);
    if (it != subnets_by_type_.end()) {
        for (const auto& cidr : it->second) {
            auto subnet_it = subnets_by_cidr_.find(cidr);
            if (subnet_it != subnets_by_cidr_.end()) {
                result.push_back(subnet_it->second);
            }
        }
    }
    
    return result;
}

bool NetworkConfigEditor::configure_vpc(const std::string& vpc_id,
                                       const std::string& vps_instance_id,
                                       const std::string& region,
                                       const std::string& availability_zone) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (!current_profile_) {
        current_profile_ = std::make_unique<NetworkProfile>("default");
    }
    
    current_profile_->vpc_id = vpc_id;
    current_profile_->vps_instance_id = vps_instance_id;
    current_profile_->availability_zone = availability_zone;
    
    // Update all VPC subnets with VPC ID
    for (auto& [cidr, config] : subnets_by_cidr_) {
        if (config.type == NetworkType::VPC) {
            config.vpc_id = vpc_id;
            config.vps_instance_id = vps_instance_id;
            config.region = region;
        }
    }
    
    if (current_profile_) {
        for (auto& subnet : current_profile_->subnets) {
            if (subnet.type == NetworkType::VPC) {
                subnet.vpc_id = vpc_id;
                subnet.vps_instance_id = vps_instance_id;
                subnet.region = region;
            }
        }
    }
    
    return true;
}

bool NetworkConfigEditor::add_vpc_subnet(const std::string& cidr,
                                        const std::string& vpc_id,
                                        const std::string& name) {
    if (!add_subnet(cidr, NetworkType::VPC, name)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::string cidr_key = cidr;
    auto it = subnets_by_cidr_.find(cidr_key);
    if (it != subnets_by_cidr_.end()) {
        it->second.vpc_id = vpc_id;
        
        if (current_profile_) {
            for (auto& subnet : current_profile_->subnets) {
                if (subnet.to_cidr() == cidr) {
                    subnet.vpc_id = vpc_id;
                    break;
                }
            }
        }
    }
    
    return true;
}

bool NetworkConfigEditor::add_vpn_subnet(const std::string& cidr,
                                        const std::string& endpoint,
                                        const std::string& protocol,
                                        const std::string& name) {
    if (!add_subnet(cidr, NetworkType::VPN, name)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::string cidr_key = cidr;
    auto it = subnets_by_cidr_.find(cidr_key);
    if (it != subnets_by_cidr_.end()) {
        it->second.vpn_endpoint = endpoint;
        it->second.vpn_protocol = protocol;
        it->second.require_encryption = true;
        it->second.require_authentication = true;
        
        if (current_profile_) {
            for (auto& subnet : current_profile_->subnets) {
                if (subnet.to_cidr() == cidr) {
                    subnet.vpn_endpoint = endpoint;
                    subnet.vpn_protocol = protocol;
                    subnet.require_encryption = true;
                    subnet.require_authentication = true;
                    break;
                }
            }
        }
    }
    
    return true;
}

bool NetworkConfigEditor::add_vnc_subnet(const std::string& cidr,
                                        uint16_t port,
                                        bool encrypted,
                                        const std::string& name) {
    if (!add_subnet(cidr, NetworkType::VNC, name)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::string cidr_key = cidr;
    auto it = subnets_by_cidr_.find(cidr_key);
    if (it != subnets_by_cidr_.end()) {
        it->second.vnc_port = port;
        it->second.vnc_encrypted = encrypted;
        it->second.require_encryption = encrypted;
        
        if (current_profile_) {
            for (auto& subnet : current_profile_->subnets) {
                if (subnet.to_cidr() == cidr) {
                    subnet.vnc_port = port;
                    subnet.vnc_encrypted = encrypted;
                    subnet.require_encryption = encrypted;
                    break;
                }
            }
        }
    }
    
    return true;
}

bool NetworkConfigEditor::create_profile(const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (profiles_.find(name) != profiles_.end()) {
        return false; // Profile already exists
    }
    
    NetworkProfile profile(name);
    profile.description = description;
    profiles_[name] = profile;
    
    return true;
}

bool NetworkConfigEditor::load_profile(const std::string& name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = profiles_.find(name);
    if (it == profiles_.end()) {
        return false;
    }
    
    current_profile_ = std::make_unique<NetworkProfile>(it->second);
    
    // Rebuild subnet indexes
    subnets_by_cidr_.clear();
    subnets_by_type_.clear();
    
    for (const auto& subnet : current_profile_->subnets) {
        std::string cidr = subnet.to_cidr();
        subnets_by_cidr_.emplace(cidr, subnet);
    }
    
    update_subnet_indexes();
    return true;
}

bool NetworkConfigEditor::save_profile(const std::string& name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (!current_profile_) {
        return false;
    }
    
    current_profile_->profile_name = name;
    current_profile_->updated_at = std::chrono::system_clock::now();
    profiles_[name] = *current_profile_;
    
    return true;
}

bool NetworkConfigEditor::delete_profile(const std::string& name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = profiles_.find(name);
    if (it == profiles_.end()) {
        return false;
    }
    
    profiles_.erase(it);
    return true;
}

std::vector<std::string> NetworkConfigEditor::list_profiles() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::vector<std::string> result;
    result.reserve(profiles_.size());
    
    for (const auto& [name, profile] : profiles_) {
        result.push_back(name);
    }
    
    return result;
}

bool NetworkConfigEditor::add_subnet_config(const SubnetConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (!validate_subnet(config)) {
        return false;
    }
    
    std::string cidr = config.to_cidr();
    subnets_by_cidr_.emplace(cidr, config);
    
    if (current_profile_) {
        current_profile_->subnets.push_back(config);
    }
    
    update_subnet_indexes();
    return true;
}

std::vector<SubnetConfig> NetworkConfigEditor::get_all_subnets() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::vector<SubnetConfig> result;
    result.reserve(subnets_by_cidr_.size());
    
    for (const auto& pair : subnets_by_cidr_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::optional<NetworkProfile> NetworkConfigEditor::get_current_profile() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (current_profile_) {
        return *current_profile_;
    }
    
    return {};
}

std::string NetworkConfigEditor::validate_configuration() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    // Check for overlapping subnets
    // Check for invalid configurations
    // Return empty string if valid, error message otherwise
    
    return ""; // TODO: Implement validation
}

void NetworkConfigEditor::clear() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    subnets_by_cidr_.clear();
    subnets_by_type_.clear();
    current_profile_ = std::make_unique<NetworkProfile>("default");
}

bool NetworkConfigEditor::load_from_file(const std::string& filepath [[maybe_unused]]) {
    // TODO: Implement file loading
    return false;
}

bool NetworkConfigEditor::save_to_file(const std::string& filepath [[maybe_unused]]) const {
    // TODO: Implement file saving
    return false;
}

std::string NetworkConfigEditor::export_to_json() const {
    // TODO: Implement JSON export
    return "{}";
}

bool NetworkConfigEditor::import_from_json(const std::string& json [[maybe_unused]]) {
    // TODO: Implement JSON import
    return false;
}

size_t NetworkConfigEditor::get_subnet_count(NetworkType type) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = subnets_by_type_.find(type);
    if (it == subnets_by_type_.end()) {
        return 0;
    }
    
    return it->second.size();
}

size_t NetworkConfigEditor::get_total_subnet_count() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return subnets_by_cidr_.size();
}

bool NetworkConfigEditor::is_empty() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return subnets_by_cidr_.empty();
}

std::string NetworkConfigEditor::generate_subnet_name(const std::string& cidr, NetworkType type) const {
    std::ostringstream oss;
    oss << NetworkTypeHelper::to_string(type) << "-" << cidr;
    return oss.str();
}

void NetworkConfigEditor::update_subnet_indexes() {
    subnets_by_type_.clear();
    
    for (const auto& pair : subnets_by_cidr_) {
        subnets_by_type_[pair.second.type].push_back(pair.first);
    }
}

bool NetworkConfigEditor::validate_subnet(const SubnetConfig& config) const {
    // Basic validation
    if (config.prefix_length == 0) {
        return false;
    }
    
    if (config.is_ipv6 && config.prefix_length > 128) {
        return false;
    }
    
    if (!config.is_ipv6 && config.prefix_length > 32) {
        return false;
    }
    
    return true;
}

// ============================================================================
// NetworkTypeHelper Implementation
// ============================================================================

namespace NetworkTypeHelper {

std::string to_string(NetworkType type) {
    switch (type) {
        case NetworkType::PUBLIC: return "PUBLIC";
        case NetworkType::PRIVATE: return "PRIVATE";
        case NetworkType::VPN: return "VPN";
        case NetworkType::VNC: return "VNC";
        case NetworkType::VPC: return "VPC";
        case NetworkType::WAN: return "WAN";
        case NetworkType::LOCAL: return "LOCAL";
        case NetworkType::DMZ: return "DMZ";
        case NetworkType::TUNNEL: return "TUNNEL";
        case NetworkType::BRIDGE: return "BRIDGE";
        default: return "UNKNOWN";
    }
}

std::optional<NetworkType> from_string(const std::string& str) {
    if (str == "PUBLIC") return NetworkType::PUBLIC;
    if (str == "PRIVATE") return NetworkType::PRIVATE;
    if (str == "VPN") return NetworkType::VPN;
    if (str == "VNC") return NetworkType::VNC;
    if (str == "VPC") return NetworkType::VPC;
    if (str == "WAN") return NetworkType::WAN;
    if (str == "LOCAL") return NetworkType::LOCAL;
    if (str == "DMZ") return NetworkType::DMZ;
    if (str == "TUNNEL") return NetworkType::TUNNEL;
    if (str == "BRIDGE") return NetworkType::BRIDGE;
    return std::nullopt;
}

std::string get_description(NetworkType type) {
    switch (type) {
        case NetworkType::PUBLIC: return "Public internet networks";
        case NetworkType::PRIVATE: return "Private internal networks";
        case NetworkType::VPN: return "Virtual Private Network";
        case NetworkType::VNC: return "Virtual Network Computing";
        case NetworkType::VPC: return "Virtual Private Cloud (for VPS)";
        case NetworkType::WAN: return "Wide Area Network";
        case NetworkType::LOCAL: return "Localhost/local networks";
        case NetworkType::DMZ: return "Demilitarized Zone";
        case NetworkType::TUNNEL: return "Network tunnels";
        case NetworkType::BRIDGE: return "Network bridges";
        default: return "Unknown network type";
    }
}

} // namespace NetworkTypeHelper

} // namespace config
} // namespace network
} // namespace dualstack