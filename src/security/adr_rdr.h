#pragma once

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

#include "signature_visualizer.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include <cstddef>

namespace dualstack::security::visualization {

// ADR-RDR (Amphisbaena Digital Reader)
// Reader component for ADS-SIG visual signatures

class ADRReader {
private:
    std::unique_ptr<SignatureVisualizer> visualizer_;
    SignatureVisualizer::SecureDataReader secure_reader_;
    
public:
    // Reader configuration
    struct ReaderConfig {
        bool verify_integrity = true;
        bool decrypt_embedded_data = true;
        bool extract_biometrics = true;
        bool extract_domain_info = true;
        float authentication_tolerance = 0.1f;
    };
    
    // Read results
    struct ReadResult {
        bool success;
        std::string error_message;
        VisualSignature signature;
        std::vector<std::byte> decrypted_data;
        BiometricData biometric_info;
        DomainVerification domain_info;
        std::map<std::string, float> security_metrics;
        bool is_authenticated;
    };
    
    explicit ADRReader(const std::vector<std::byte>& kyber_private_key,
                      const std::vector<std::byte>& aes_key,
                      ReaderConfig config = ReaderConfig{})
        : visualizer_(std::make_unique<SignatureVisualizer>()),
          secure_reader_(kyber_private_key, aes_key) {
        visualizer_->set_kyber_keys({}, kyber_private_key); // Private key for reader
        visualizer_->set_aes_key(aes_key);
    }
    
    // Read visual signature from file (SVG, PNG, etc.)
    auto read_signature_file(const std::string& filepath) -> ReadResult;
    
    // Read visual signature from string representation
    auto read_signature_string(const std::string& signature_str) -> ReadResult;
    
    // Read visual signature from image data
    auto read_signature_image(const std::vector<std::byte>& image_data) -> ReadResult;
    
    // Authenticate signature against reference
    auto authenticate_signature(const VisualSignature& sig,
                               const VisualSignature& reference) -> bool;
    
    // Extract and decrypt all embedded information
    auto extract_information(const VisualSignature& sig) -> ReadResult;
    
    // Verify signature integrity
    auto verify_signature_integrity(const VisualSignature& sig) -> bool;
    
    // Get security metrics for the signature
    auto get_security_analysis(const VisualSignature& sig) -> std::map<std::string, float>;
    
    // Export signature data in various formats
    auto export_signature_data(const VisualSignature& sig, const std::string& format) -> std::vector<std::byte>;
    
    // Validate signature format and structure
    auto validate_signature_format(const VisualSignature& sig) -> bool;
    
    // Compare two signatures
    auto compare_signatures(const VisualSignature& sig1, const VisualSignature& sig2) -> float;
    
private:
    // Internal helper methods
    auto parse_svg_file(const std::string& filepath) -> VisualSignature;
    auto parse_png_data(const std::vector<std::byte>& data) -> VisualSignature;
    auto extract_visual_data_from_image(const std::vector<std::byte>& image_data) -> VisualSignature;
    auto calculate_similarity(const VisualSignature& sig1, const VisualSignature& sig2) -> float;
};

// Command-line interface for ADR-RDR
class ADRCommandLineInterface {
public:
    static auto run_reader(int argc, char* argv[]) -> int;
    static auto display_help() -> void;
    static auto display_signature_info(const ADRReader::ReadResult& result) -> void;
    static auto export_results(const ADRReader::ReadResult& result, const std::string& output_path) -> bool;
};

// GUI interface for ADR-RDR (placeholder for future implementation)
class ADRGUIInterface {
public:
    // GUI methods would be implemented here
    static auto launch_gui() -> void;
    static auto display_signature_visualization(const VisualSignature& sig) -> void;
};

} // namespace dualstack::security::visualization