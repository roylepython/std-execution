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

using dualstack::security::visualization::SignatureVisualizer;
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include <cstddef>
#include <optional>

// Define std::byte if not available
#if __cplusplus >= 201703L
    // C++17 std::byte is available
    #include <cstddef>
#else
    // Fallback for older standards
    namespace std {
        enum class byte : unsigned char {};
    }
#endif

namespace dualstack::security::visualization {

// ADR-RDR (Amphisbaena Digital Reader)
// Reader component for ADS-SIG visual signatures

class ADRReader {
private:
    std::unique_ptr<SignatureVisualizer> visualizer_;
    std::optional<SignatureVisualizer::SecureDataReader> secure_reader_;
    
public:
    // Reader configuration
    struct ReaderConfig {
        bool verify_integrity;
        bool decrypt_embedded_data;
        bool extract_biometrics;
        bool extract_domain_info;
        float authentication_tolerance;
        
        // Constructor to initialize members
        ReaderConfig() : 
            verify_integrity(true),
            decrypt_embedded_data(true),
            extract_biometrics(true),
            extract_domain_info(true),
            authentication_tolerance(0.1f) {}
    };
    
    // Read results
    struct ReadResult {
        bool success;
        std::string error_message;
        SignatureVisualizer::VisualSignature signature;
        std::vector<std::byte> decrypted_data;
        SignatureVisualizer::BiometricData biometric_info;
        SignatureVisualizer::DomainVerification domain_info;
        std::map<std::string, float> security_metrics;
        bool is_authenticated;
    };
    
    explicit ADRReader(const std::vector<std::byte>& kyber_private_key,
                      const std::vector<std::byte>& aes_key,
                      ReaderConfig config = ReaderConfig{})
        : visualizer_(std::make_unique<SignatureVisualizer>()),
          secure_reader_(std::make_optional<SignatureVisualizer::SecureDataReader>(kyber_private_key, aes_key)) {
        visualizer_->set_kyber_keys({}, kyber_private_key); // Private key for reader
        visualizer_->set_aes_key(aes_key);
        // Suppress unused parameter warning
        (void)config;
    }
    
    // Read visual signature from file (SVG, PNG, etc.)
    auto read_signature_file(const std::string& filepath) -> ReadResult;
    
    // Read visual signature from string representation
    auto read_signature_string(const std::string& signature_str) -> ReadResult;
    
    // Read visual signature from image data
    auto read_signature_image(const std::vector<std::byte>& image_data) -> ReadResult;
    
    // Authenticate signature against reference
    auto authenticate_signature(const SignatureVisualizer::VisualSignature& sig,
                               const SignatureVisualizer::VisualSignature& reference) -> bool;
    
    // Extract and decrypt all embedded information
    auto extract_information(const SignatureVisualizer::VisualSignature& sig) -> ReadResult;
    
    // Verify signature integrity
    auto verify_signature_integrity(const SignatureVisualizer::VisualSignature& sig) -> bool;
    
    // Get security metrics for the signature
    auto get_security_analysis(const SignatureVisualizer::VisualSignature& sig) -> std::map<std::string, float>;
    
    // Export signature data in various formats
    auto export_signature_data(const SignatureVisualizer::VisualSignature& sig, const std::string& format) -> std::vector<std::byte>;
    
    // Validate signature format and structure
    auto validate_signature_format(const SignatureVisualizer::VisualSignature& sig) -> bool;
    
    // Compare two signatures
    auto compare_signatures(const SignatureVisualizer::VisualSignature& sig1, const SignatureVisualizer::VisualSignature& sig2) -> float;
    
private:
    // Internal helper methods
    auto parse_svg_file(const std::string& filepath) -> SignatureVisualizer::VisualSignature;
    auto parse_png_data(const std::vector<std::byte>& data) -> SignatureVisualizer::VisualSignature;
    auto extract_visual_data_from_image(const std::vector<std::byte>& image_data) -> SignatureVisualizer::VisualSignature;
    auto calculate_similarity(const SignatureVisualizer::VisualSignature& sig1, const SignatureVisualizer::VisualSignature& sig2) -> float;
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
    static auto display_signature_visualization(const SignatureVisualizer::VisualSignature& sig) -> void;
};

} // namespace dualstack::security::visualization