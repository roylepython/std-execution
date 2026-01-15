/**
 * Amphisbaena 🐍 - Enhanced Network Configuration System
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * PUBLIC LIBRARY - EASY-TO-USE NETWORK CONFIGURATION
 * 
 * Comprehensive network configuration for:
 * - Public Networks
 * - Private Networks
 * - VPN (Virtual Private Network)
 * - VNC (Virtual Network Computing)
 * - VPC (Virtual Private Cloud) for VPS
 * - WAN (Wide Area Network)
 * - LOCAL Networks
 * - All Subnets
 * 
 * The name of the game is EASY TO USE!
 * 
 * Yorkshire Champion Standards - Improving AI Safety and the Web
 */

#pragma once

// Include format header fix BEFORE any standard headers to prevent GCC 14.2.0 format header bug
#include "../fix_format_header.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <expected>
#include "../../../src/core/ip_address.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <functional>

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
namespace config {

/**
 * @brief Network Type Enumeration
 * 
 * Comprehensive network types supported by Amphisbaena
 */
enum class NetworkType : uint8_t {
    PUBLIC = 0,      // Public internet networks
    PRIVATE = 1,     // Private internal networks
    VPN = 2,         // Virtual Private Network
    VNC = 3,         // Virtual Network Computing
    VPC = 4,         // Virtual Private Cloud (for VPS)
    WAN = 5,         // Wide Area Network
    LOCAL = 6,       // Localhost/local networks
    DMZ = 7,         // Demilitarized Zone
    TUNNEL = 8,      // Network tunnels
    BRIDGE = 9       // Network bridges
};

/**
 * @brief Subnet Configuration
 * 
 * Represents a network subnet with CIDR notation
 */
struct AMPHISBAENA_API SubnetConfig {
    std::string name;                    // Human-readable name
    IPAddress network_address;           // Network address
    uint8_t prefix_length;               // CIDR prefix length (e.g., /24)
    NetworkType type;                    // Network type
    bool is_ipv6;                        // IPv6 subnet flag
    
    // Subnet properties
    bool allow_inbound;                  // Allow inbound connections
    bool allow_outbound;                 // Allow outbound connections
    bool require_encryption;             // Require encryption
    bool require_authentication;         // Require authentication
    
    // VPC/VPS specific
    std::string vpc_id;                  // VPC identifier (if applicable)
    std::string vps_instance_id;         // VPS instance identifier (if applicable)
    std::string region;                  // Geographic region
    
    // VPN specific
    std::string vpn_endpoint;            // VPN endpoint address
    std::string vpn_protocol;            // VPN protocol (OpenVPN, WireGuard, etc.)
    
    // VNC specific
    uint16_t vnc_port;                   // VNC port (default 5900)
    bool vnc_encrypted;                   // VNC encryption enabled
    
    // Metadata
    std::string description;             // Description
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    SubnetConfig();
    SubnetConfig(const std::string& name, const IPAddress& addr, uint8_t prefix, NetworkType type);
    
    // Helper methods
    bool contains(const IPAddress& addr) const;
    std::string to_cidr() const;
    static std::expected<SubnetConfig, std::string> from_cidr(const std::string& cidr, NetworkType type);
};

/**
 * @brief Network Interface Configuration
 * 
 * Represents a physical or virtual network interface
 */
struct AMPHISBAENA_API InterfaceConfig {
    std::string name;                    // Interface name (e.g., eth0, wlan0)
    std::string mac_address;            // MAC address
    IPAddress ip_address;               // Primary IP address
    IPAddress subnet_mask;              // Subnet mask
    IPAddress gateway;                  // Default gateway
    std::vector<IPAddress> dns_servers;  // DNS servers
    bool is_up;                         // Interface is up
    bool is_loopback;                   // Loopback interface
    NetworkType primary_type;           // Primary network type
    
    // Interface properties
    uint64_t mtu;                       // Maximum Transmission Unit
    uint64_t speed_mbps;                // Interface speed (Mbps)
    bool promiscuous_mode;              // Promiscuous mode enabled
    
    InterfaceConfig();
};

/**
 * @brief Network Route Configuration
 * 
 * Represents a network routing rule
 */
struct AMPHISBAENA_API RouteConfig {
    std::string name;                   // Route name
    IPAddress destination;               // Destination network
    uint8_t destination_prefix;         // Destination prefix length
    IPAddress gateway;                  // Gateway address
    std::string interface_name;         // Interface name
    uint32_t metric;                    // Route metric (lower = preferred)
    bool is_default;                    // Default route
    
    RouteConfig();
};

/**
 * @brief Network Configuration Profile
 * 
 * Complete network configuration profile
 */
struct AMPHISBAENA_API NetworkProfile {
    std::string profile_name;           // Profile name
    std::string description;            // Profile description
    
    // Network components
    std::vector<SubnetConfig> subnets;   // Configured subnets
    std::vector<InterfaceConfig> interfaces; // Network interfaces
    std::vector<RouteConfig> routes;    // Routing rules
    
    // Security settings
    bool firewall_enabled;              // Firewall enabled
    bool nat_enabled;                    // NAT enabled
    bool ip_forwarding_enabled;         // IP forwarding enabled
    
    // VPC/VPS settings
    std::string vpc_id;                  // VPC identifier
    std::string vps_instance_id;         // VPS instance identifier
    std::string availability_zone;      // Availability zone
    
    // Metadata
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    NetworkProfile();
    NetworkProfile(const std::string& name);
};

/**
 * @brief Network Configuration Editor
 * 
 * Easy-to-use network configuration management system
 * 
 * The name of the game is EASY TO USE!
 */
class AMPHISBAENA_API NetworkConfigEditor {
public:
    NetworkConfigEditor();
    ~NetworkConfigEditor();
    
    // ========================================================================
    // EASY-TO-USE API - Simple methods for common operations
    // ========================================================================
    
    /**
     * @brief Add a subnet (easy method)
     * 
     * @param cidr CIDR notation (e.g., "192.168.1.0/24")
     * @param type Network type
     * @param name Optional name
     * @return true if successful
     */
    bool add_subnet(const std::string& cidr, NetworkType type, const std::string& name = "");
    
    /**
     * @brief Remove a subnet
     * 
     * @param cidr CIDR notation
     * @return true if removed
     */
    bool remove_subnet(const std::string& cidr);
    
    /**
     * @brief Get subnet for an IP address
     * 
     * @param addr IP address
     * @return Subnet config if found
     */
    std::optional<SubnetConfig> get_subnet_for(const IPAddress& addr) const;
    
    /**
     * @brief Check if IP address is in a specific network type
     * 
     * @param addr IP address
     * @param type Network type
     * @return true if in network type
     */
    bool is_in_network_type(const IPAddress& addr, NetworkType type) const;
    
    /**
     * @brief Get all subnets of a specific type
     * 
     * @param type Network type
     * @return Vector of subnet configs
     */
    std::vector<SubnetConfig> get_subnets_by_type(NetworkType type) const;
    
    // ========================================================================
    // VPC/VPS Configuration
    // ========================================================================
    
    /**
     * @brief Configure VPC for VPS
     * 
     * @param vpc_id VPC identifier
     * @param vps_instance_id VPS instance identifier
     * @param region Geographic region
     * @param availability_zone Availability zone
     * @return true if successful
     */
    bool configure_vpc(const std::string& vpc_id, 
                      const std::string& vps_instance_id,
                      const std::string& region,
                      const std::string& availability_zone);
    
    /**
     * @brief Add VPC subnet
     * 
     * @param cidr CIDR notation
     * @param vpc_id VPC identifier
     * @param name Subnet name
     * @return true if successful
     */
    bool add_vpc_subnet(const std::string& cidr, 
                       const std::string& vpc_id,
                       const std::string& name = "");
    
    // ========================================================================
    // VPN Configuration
    // ========================================================================
    
    /**
     * @brief Add VPN subnet
     * 
     * @param cidr CIDR notation
     * @param endpoint VPN endpoint
     * @param protocol VPN protocol
     * @param name Subnet name
     * @return true if successful
     */
    bool add_vpn_subnet(const std::string& cidr,
                       const std::string& endpoint,
                       const std::string& protocol = "OpenVPN",
                       const std::string& name = "");
    
    // ========================================================================
    // VNC Configuration
    // ========================================================================
    
    /**
     * @brief Add VNC subnet
     * 
     * @param cidr CIDR notation
     * @param port VNC port (default 5900)
     * @param encrypted Enable encryption
     * @param name Subnet name
     * @return true if successful
     */
    bool add_vnc_subnet(const std::string& cidr,
                       uint16_t port = 5900,
                       bool encrypted = true,
                       const std::string& name = "");
    
    // ========================================================================
    // Profile Management
    // ========================================================================
    
    /**
     * @brief Create a new network profile
     * 
     * @param name Profile name
     * @param description Profile description
     * @return true if created
     */
    bool create_profile(const std::string& name, const std::string& description = "");
    
    /**
     * @brief Load a network profile
     * 
     * @param name Profile name
     * @return true if loaded
     */
    bool load_profile(const std::string& name);
    
    /**
     * @brief Save current configuration as a profile
     * 
     * @param name Profile name
     * @return true if saved
     */
    bool save_profile(const std::string& name);
    
    /**
     * @brief Delete a profile
     * 
     * @param name Profile name
     * @return true if deleted
     */
    bool delete_profile(const std::string& name);
    
    /**
     * @brief Get all profile names
     * 
     * @return Vector of profile names
     */
    std::vector<std::string> list_profiles() const;
    
    // ========================================================================
    // Advanced Configuration
    // ========================================================================
    
    /**
     * @brief Add subnet with full configuration
     * 
     * @param config Subnet configuration
     * @return true if added
     */
    bool add_subnet_config(const SubnetConfig& config);
    
    /**
     * @brief Get all subnets
     * 
     * @return Vector of all subnet configs
     */
    std::vector<SubnetConfig> get_all_subnets() const;
    
    /**
     * @brief Get current network profile
     * 
     * @return Current profile or nullopt
     */
    std::optional<NetworkProfile> get_current_profile() const;
    
    /**
     * @brief Validate configuration
     * 
     * @return Empty string if valid, error message otherwise
     */
    std::string validate_configuration() const;
    
    /**
     * @brief Clear all configuration
     */
    void clear();
    
    // ========================================================================
    // Persistence
    // ========================================================================
    
    /**
     * @brief Load configuration from file
     * 
     * @param filepath File path
     * @return true if loaded
     */
    bool load_from_file(const std::string& filepath);
    
    /**
     * @brief Save configuration to file
     * 
     * @param filepath File path
     * @return true if saved
     */
    bool save_to_file(const std::string& filepath) const;
    
    /**
     * @brief Export configuration to JSON
     * 
     * @return JSON string
     */
    std::string export_to_json() const;
    
    /**
     * @brief Import configuration from JSON
     * 
     * @param json JSON string
     * @return true if imported
     */
    bool import_from_json(const std::string& json);
    
    // ========================================================================
    // Statistics and Information
    // ========================================================================
    
    /**
     * @brief Get subnet count by type
     * 
     * @param type Network type
     * @return Count
     */
    size_t get_subnet_count(NetworkType type) const;
    
    /**
     * @brief Get total subnet count
     * 
     * @return Total count
     */
    size_t get_total_subnet_count() const;
    
    /**
     * @brief Check if configuration is empty
     * 
     * @return true if empty
     */
    bool is_empty() const;

private:
    mutable std::mutex config_mutex_;
    
    // Current configuration
    std::unique_ptr<NetworkProfile> current_profile_;
    
    // Subnet lookup by CIDR
    std::unordered_map<std::string, SubnetConfig> subnets_by_cidr_;
    
    // Subnet lookup by network type
    std::unordered_map<NetworkType, std::vector<std::string>> subnets_by_type_;
    
    // All profiles
    std::unordered_map<std::string, NetworkProfile> profiles_;
    
    // Helper methods
    std::string generate_subnet_name(const std::string& cidr, NetworkType type) const;
    void update_subnet_indexes();
    bool validate_subnet(const SubnetConfig& config) const;
};

/**
 * @brief Network Type Helper Functions
 */
namespace NetworkTypeHelper {
    /**
     * @brief Convert network type to string
     * 
     * @param type Network type
     * @return String representation
     */
    AMPHISBAENA_API std::string to_string(NetworkType type);
    
    /**
     * @brief Convert string to network type
     * 
     * @param str String representation
     * @return Network type or nullopt
     */
    AMPHISBAENA_API std::optional<NetworkType> from_string(const std::string& str);
    
    /**
     * @brief Get network type description
     * 
     * @param type Network type
     * @return Description
     */
    AMPHISBAENA_API std::string get_description(NetworkType type);
}

} // namespace config
} // namespace network
} // namespace dualstack