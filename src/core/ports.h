/**
 * Amphisbaena 🐍 - Port Configuration
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * Standard port assignments for Amphisbaena ecosystem
 * 
 * Yorkshire Champion Standards - Improving AI Safety and the Web
 */

#pragma once

#include <cstdint>

namespace dualstack {
namespace network {

/**
 * @brief Standard Port Assignments
 * 
 * Port assignments for the Amphisbaena ecosystem:
 * - HTTPS: 443 (standard)
 * - GalaxyCDN: 886 (management connection, piggybacks BitTorrent ports for legal use demonstration)
 * - Amphisbaena ADS-RDR: 42 (the answer to life, the universe, and everything)
 * - Key Service: 84 (key management and distribution)
 */
namespace Ports {
    // Standard ports
    constexpr uint16_t HTTPS = 443;
    
    // Amphisbaena ecosystem ports
    constexpr uint16_t GALAXYCDN_MANAGEMENT = 886;      // GalaxyCDN management connection
    constexpr uint16_t AMPHISBAENA_ADS_RDR = 42;        // Amphisbaena ADS-RDR service (the answer!)
    constexpr uint16_t KEY_SERVICE = 84;                 // Key management service
    
    // Port ranges for dynamic allocation
    constexpr uint16_t DYNAMIC_START = 49152;
    constexpr uint16_t DYNAMIC_END = 65535;
    
    /**
     * @brief Validate port number
     * 
     * @param port Port number to validate
     * @return true if port is valid (1-65535)
     */
    constexpr bool is_valid(uint16_t port) {
        return port > 0 && port <= 65535;
    }
    
    /**
     * @brief Check if port is in dynamic range
     * 
     * @param port Port number to check
     * @return true if port is in dynamic/ephemeral range
     */
    constexpr bool is_dynamic(uint16_t port) {
        return port >= DYNAMIC_START && port <= DYNAMIC_END;
    }
    
    /**
     * @brief Check if port is a well-known port (< 1024)
     * 
     * @param port Port number to check
     * @return true if port is well-known
     */
    constexpr bool is_well_known(uint16_t port) {
        return port > 0 && port < 1024;
    }
    
    /**
     * @brief Security note for port 42
     * 
     * Port 42 is assigned to WINS (Windows Internet Name Service).
     * Security considerations:
     * - Ensure proper firewall rules
     * - Use encryption (TLS/SSL)
     * - Monitor for unauthorized access
     * - Keep services updated
     * 
     * For Amphisbaena ADS-RDR, we use this port with:
     * - Full TLS encryption
     * - Access control lists
     * - Rate limiting
     * - Comprehensive logging
     */
    constexpr uint16_t WINS_PORT = 42;
    constexpr bool AMPHISBAENA_ADS_RDR_SECURED = true;  // Always use encryption on port 42
}

} // namespace network
} // namespace dualstack

