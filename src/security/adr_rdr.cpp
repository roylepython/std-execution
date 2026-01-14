#include "adr_rdr.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace dualstack::security::visualization {

// ADRReader implementation
auto ADRReader::read_signature_file(const std::string& filepath) -> ReadResult {
    ReadResult result{};
    
    try {
        // Try to read as SVG first
        std::ifstream file(filepath);
        if (!file.is_open()) {
            result.success = false;
            result.error_message = "Failed to open file: " + filepath;
            return result;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        auto signature = parse_svg_file(filepath);
        return extract_information(signature);
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Exception reading file: " + std::string(e.what());
        return result;
    }
}

auto ADRReader::read_signature_string(const std::string& signature_str) -> ReadResult {
    ReadResult result{};
    
    try {
        // Parse the signature string
        VisualSignature sig = visualizer_->from_string(signature_str);
        return extract_information(sig);
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Exception parsing signature: " + std::string(e.what());
        return result;
    }
}

auto ADRReader::read_signature_image(const std::vector<std::byte>& image_data) -> ReadResult {
    ReadResult result{};
    
    try {
        // Parse image data
        VisualSignature sig = extract_visual_data_from_image(image_data);
        return extract_information(sig);
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Exception parsing image: " + std::string(e.what());
        return result;
    }
}

auto ADRReader::authenticate_signature(const VisualSignature& sig,
                                     const VisualSignature& reference) -> bool {
    try {
        // Calculate similarity score
        float similarity = calculate_similarity(sig, reference);
        
        // Authentication tolerance from config
        return similarity >= 0.9f; // 90% similarity required
    } catch (...) {
        return false;
    }
}

auto ADRReader::extract_information(const VisualSignature& sig) -> ReadResult {
    ReadResult result{};
    result.success = true;
    result.signature = sig;
    
    try {
        // Verify integrity first
        if (!verify_signature_integrity(sig)) {
            result.success = false;
            result.error_message = "Signature integrity verification failed";
            return result;
        }
        
        // Decrypt embedded data if requested
        if (secure_reader_) {
            result.decrypted_data = secure_reader_.decrypt_embedded_data(sig.encrypted_metadata);
            result.biometric_info = secure_reader_.extract_biometric(sig);
            result.domain_info = secure_reader_.extract_domain_verification(sig);
        }
        
        // Perform authentication check
        result.is_authenticated = verify_signature_integrity(sig);
        
        // Get security metrics
        result.security_metrics = get_security_analysis(sig);
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Error extracting information: " + std::string(e.what());
    }
    
    return result;
}

auto ADRReader::verify_signature_integrity(const VisualSignature& sig) -> bool {
    if (secure_reader_) {
        return secure_reader_.verify_integrity(sig);
    }
    return false;
}

auto ADRReader::get_security_analysis(const VisualSignature& sig) -> std::map<std::string, float> {
    std::map<std::string, float> metrics;
    
    // Calculate various security metrics
    metrics["complexity"] = static_cast<float>(sig.complexity) / 1000.0f;
    metrics["visual_checksum_match"] = verify_signature_integrity(sig) ? 1.0f : 0.0f;
    metrics["encrypted_data_present"] = sig.encrypted_metadata.empty() ? 0.0f : 1.0f;
    metrics["biometric_data_present"] = sig.biometric_info.minutiae_points.empty() ? 0.0f : 1.0f;
    metrics["domain_verification_present"] = sig.domain_info.domain_name.empty() ? 0.0f : 1.0f;
    
    return metrics;
}

auto ADRReader::export_signature_data(const VisualSignature& sig, const std::string& format) -> std::vector<std::byte> {
    std::vector<std::byte> result;
    
    if (format == "svg") {
        std::string svg = visualizer_->to_svg(sig);
        result.assign(svg.begin(), svg.end());
    } else if (format == "string") {
        std::string str = visualizer_->to_string(sig);
        result.assign(str.begin(), str.end());
    }
    // Add more formats as needed
    
    return result;
}

auto ADRReader::validate_signature_format(const VisualSignature& sig) -> bool {
    // Basic validation checks
    return !sig.points.empty() && !sig.colors.empty() && sig.visual_checksum != 0;
}

auto ADRReader::compare_signatures(const VisualSignature& sig1, const VisualSignature& sig2) -> float {
    return calculate_similarity(sig1, sig2);
}

// Private helper methods
auto ADRReader::parse_svg_file(const std::string& filepath) -> VisualSignature {
    // Placeholder implementation - in reality this would parse SVG content
    VisualSignature sig;
    // Parse SVG and extract visual signature data
    return sig;
}

auto ADRReader::parse_png_data(const std::vector<std::byte>& data) -> VisualSignature {
    // Placeholder implementation - in reality this would parse PNG content
    VisualSignature sig;
    // Parse PNG and extract visual signature data
    return sig;
}

auto ADRReader::extract_visual_data_from_image(const std::vector<std::byte>& image_data) -> VisualSignature {
    // Placeholder implementation - detect image format and parse accordingly
    if (image_data.size() > 8 && 
        static_cast<unsigned char>(image_data[0]) == 0x89 &&
        static_cast<unsigned char>(image_data[1]) == 0x50) { // PNG signature
        return parse_png_data(image_data);
    }
    // Default to creating empty signature
    return VisualSignature{};
}

auto ADRReader::calculate_similarity(const VisualSignature& sig1, const VisualSignature& sig2) -> float {
    if (sig1.points.size() != sig2.points.size()) {
        return 0.0f;
    }
    
    if (sig1.points.empty()) {
        return 1.0f; // Both empty, considered identical
    }
    
    float total_distance = 0.0f;
    for (size_t i = 0; i < sig1.points.size(); ++i) {
        float dx = sig1.points[i][0] - sig2.points[i][0];
        float dy = sig1.points[i][1] - sig2.points[i][1];
        total_distance += std::sqrt(dx * dx + dy * dy);
    }
    
    float avg_distance = total_distance / sig1.points.size();
    return std::max(0.0f, 1.0f - avg_distance); // Convert distance to similarity
}

// Command-line interface implementation
auto ADRCommandLineInterface::run_reader(int argc, char* argv[]) -> int {
    if (argc < 2) {
        display_help();
        return 1;
    }
    
    std::string filepath = argv[1];
    ADRReader reader({}, {}); // Empty keys for now
    
    auto result = reader.read_signature_file(filepath);
    
    if (result.success) {
        std::cout << "✅ Signature read successfully!" << std::endl;
        display_signature_info(result);
        return 0;
    } else {
        std::cerr << "❌ Failed to read signature: " << result.error_message << std::endl;
        return 1;
    }
}

auto ADRCommandLineInterface::display_help() -> void {
    std::cout << "ADR-RDR (Amphisbaena Digital Reader)" << std::endl;
    std::cout << "Usage: adr_rdr <signature_file>" << std::endl;
    std::cout << "Supported formats: SVG, PNG" << std::endl;
}

auto ADRCommandLineInterface::display_signature_info(const ADRReader::ReadResult& result) -> void {
    std::cout << "Signature Info:" << std::endl;
    std::cout << "  Points: " << result.signature.points.size() << std::endl;
    std::cout << "  Colors: " << result.signature.colors.size() << std::endl;
    std::cout << "  Complexity: " << result.signature.complexity << std::endl;
    std::cout << "  Authenticated: " << (result.is_authenticated ? "Yes" : "No") << std::endl;
    
    if (!result.decrypted_data.empty()) {
        std::cout << "  Decrypted Data Size: " << result.decrypted_data.size() << " bytes" << std::endl;
    }
}

auto ADRCommandLineInterface::export_results(const ADRReader::ReadResult& result, const std::string& output_path) -> bool {
    try {
        std::ofstream file(output_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // Export as SVG
        std::string svg = result.signature.visualizer->to_svg(result.signature);
        file.write(svg.data(), svg.size());
        file.close();
        
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace dualstack::security::visualization