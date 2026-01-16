#include "tls_protocol.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <set>
#include <string>
#include <vector>

// Include PsiForceDB security components
#include "../../../Projects/LamiaFabrica/Back-Office/PsiForceDB_1.0.0/inc/lfssl/kyber1024.hpp"
#include "../../../Projects/LamiaFabrica/Back-Office/PsiForceDB_1.0.0/include/jwt-cpp/jwt.h"
#include "../../../Projects/LamiaFabrica/Back-Office/PsiForceDB_1.0.0/include/openssl/aes.h"

namespace dualstack::security::tls {

// Function-local lazy initialization for known bad IPs - avoids static initialization and -Wfree-nonheap-object warnings
namespace {
    const std::set<std::string>* get_known_bad_ips() {
        static const std::set<std::string>* ips = nullptr;
        if (!ips) {
            ips = new std::set<std::string>{
                "192.168.1.100", "10.0.0.50", "172.16.0.1"
            };
        }
        return ips;
    }
}

// Kyber Key Exchange Implementation
auto pqc::KyberKeyExchange::generate_keypair() -> std::pair<std::vector<std::byte>, std::vector<std::byte>> {
    std::vector<uint8_t> public_key, secret_key;
    
    if (!LFSSL::Kyber1024::keygen(public_key, secret_key)) {
        throw std::runtime_error("Failed to generate Kyber keypair");
    }
    
    // Convert to std::byte vectors
    std::vector<std::byte> pub_key(public_key.size());
    std::vector<std::byte> priv_key(secret_key.size());
    
    std::transform(public_key.begin(), public_key.end(), pub_key.begin(),
                   [](uint8_t b) { return static_cast<std::byte>(b); });
    std::transform(secret_key.begin(), secret_key.end(), priv_key.begin(),
                   [](uint8_t b) { return static_cast<std::byte>(b); });
    
    return {pub_key, priv_key};
}

auto pqc::KyberKeyExchange::encapsulate(const std::vector<std::byte>& public_key) -> std::pair<std::vector<std::byte>, std::vector<std::byte>> {
    // Convert std::byte to uint8_t
    std::vector<uint8_t> pub_key(public_key.size());
    std::transform(public_key.begin(), public_key.end(), pub_key.begin(),
                   [](std::byte b) { return static_cast<uint8_t>(b); });
    
    std::vector<uint8_t> ciphertext, shared_secret;
    
    if (!LFSSL::Kyber1024::encapsulate(pub_key, ciphertext, shared_secret)) {
        throw std::runtime_error("Failed to encapsulate with Kyber");
    }
    
    // Convert back to std::byte
    std::vector<std::byte> ct(ciphertext.size());
    std::vector<std::byte> ss(shared_secret.size());
    
    std::transform(ciphertext.begin(), ciphertext.end(), ct.begin(),
                   [](uint8_t b) { return static_cast<std::byte>(b); });
    std::transform(shared_secret.begin(), shared_secret.end(), ss.begin(),
                   [](uint8_t b) { return static_cast<std::byte>(b); });
    
    return {ct, ss};
}

auto pqc::KyberKeyExchange::decapsulate(const std::vector<std::byte>& ciphertext, const std::vector<std::byte>& private_key) -> std::vector<std::byte> {
    // Convert std::byte to uint8_t
    std::vector<uint8_t> ct(ciphertext.size());
    std::vector<uint8_t> priv_key(private_key.size());
    
    std::transform(ciphertext.begin(), ciphertext.end(), ct.begin(),
                   [](std::byte b) { return static_cast<uint8_t>(b); });
    std::transform(private_key.begin(), private_key.end(), priv_key.begin(),
                   [](std::byte b) { return static_cast<uint8_t>(b); });
    
    std::vector<uint8_t> shared_secret;
    
    if (!LFSSL::Kyber1024::decapsulate(priv_key, ct, shared_secret)) {
        throw std::runtime_error("Failed to decapsulate with Kyber");
    }
    
    // Convert back to std::byte
    std::vector<std::byte> ss(shared_secret.size());
    std::transform(shared_secret.begin(), shared_secret.end(), ss.begin(),
                   [](uint8_t b) { return static_cast<std::byte>(b); });
    
    return ss;
}

// Dilithium Signature Implementation
auto pqc::DilithiumSignature::generate_keypair() -> std::pair<std::vector<std::byte>, std::vector<std::byte>> {
    // In a real implementation, this would use the actual Dilithium algorithm
    // For now, generate random keypair
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);
    
    std::vector<std::byte> public_key(1952);  // Dilithium3 public key size
    std::vector<std::byte> private_key(4032); // Dilithium3 private key size
    
    for (auto& byte : public_key) {
        byte = static_cast<std::byte>(dis(gen));
    }
    
    for (auto& byte : private_key) {
        byte = static_cast<std::byte>(dis(gen));
    }
    
    return {public_key, private_key};
}

auto pqc::DilithiumSignature::sign(const std::vector<std::byte>& message, const std::vector<std::byte>& private_key) -> std::vector<std::byte> {
    // In a real implementation, this would use the actual Dilithium signing algorithm
    // For now, generate a deterministic signature based on message and key
    std::vector<std::byte> signature(4928); // Dilithium3 signature size
    
    // Simple hash-based approach for demonstration
    std::hash<std::string> hasher;
    std::string combined;
    
    for (const auto& byte : message) {
        combined += static_cast<char>(byte);
    }
    
    for (const auto& byte : private_key) {
        combined += static_cast<char>(byte);
    }
    
    size_t hash_value = hasher(combined);
    
    // Fill signature with hash-based values
    for (size_t i = 0; i < signature.size(); ++i) {
        signature[i] = static_cast<std::byte>((hash_value >> (i % 32)) & 0xFF);
    }
    
    return signature;
}

auto pqc::DilithiumSignature::verify(const std::vector<std::byte>& message, const std::vector<std::byte>& signature, const std::vector<std::byte>& public_key) -> bool {
    // In a real implementation, this would verify the actual Dilithium signature
    // For now, return true for demonstration purposes
    return signature.size() == 4928; // Basic size check
}

// Hybrid Crypto Implementation
auto pqc::HybridCrypto::combine_keys(const std::vector<std::byte>& pqc_key, const std::vector<std::byte>& classical_key) -> std::vector<std::byte> {
    std::vector<std::byte> combined;
    combined.reserve(pqc_key.size() + classical_key.size());
    
    combined.insert(combined.end(), pqc_key.begin(), pqc_key.end());
    combined.insert(combined.end(), classical_key.begin(), classical_key.end());
    
    return combined;
}

auto pqc::HybridCrypto::split_keys(const std::vector<std::byte>& combined_key) -> std::pair<std::vector<std::byte>, std::vector<std::byte>> {
    // Assume first half is PQC key, second half is classical key
    size_t half_size = combined_key.size() / 2;
    
    std::vector<std::byte> pqc_key(combined_key.begin(), combined_key.begin() + half_size);
    std::vector<std::byte> classical_key(combined_key.begin() + half_size, combined_key.end());
    
    return {pqc_key, classical_key};
}

// JWT Token Implementation
JWTToken::JWTToken(const std::string& token) {
    size_t first_dot = token.find('.');
    size_t second_dot = token.find('.', first_dot + 1);
    
    if (first_dot == std::string::npos || second_dot == std::string::npos) {
        throw std::invalid_argument("Invalid JWT token format");
    }
    
    header_ = token.substr(0, first_dot);
    payload_ = token.substr(first_dot + 1, second_dot - first_dot - 1);
    signature_ = token.substr(second_dot + 1);
}

auto JWTToken::create(const std::string& subject, const std::chrono::seconds& expiry, 
                      const std::map<std::string, std::string>& claims) -> JWTToken {
    using namespace jwt;
    
    auto token = create(algorithm::hs256{}, "secret_key")
        .set_subject(subject)
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + expiry);
    
    // Add custom claims
    for (const auto& [key, value] : claims) {
        token.set_payload_claim(key, value);
    }
    
    std::string jwt_string = token.sign();
    
    JWTToken jwt_token;
    size_t first_dot = jwt_string.find('.');
    size_t second_dot = jwt_string.find('.', first_dot + 1);
    
    jwt_token.header_ = jwt_string.substr(0, first_dot);
    jwt_token.payload_ = jwt_string.substr(first_dot + 1, second_dot - first_dot - 1);
    jwt_token.signature_ = jwt_string.substr(second_dot + 1);
    
    return jwt_token;
}

auto JWTToken::validate(const std::vector<std::byte>& public_key) const -> bool {
    try {
        using namespace jwt;
        
        std::string token_string = header_ + "." + payload_ + "." + signature_;
        auto decoded = decode(token_string);
        
        // Verify expiration
        auto now = std::chrono::system_clock::now();
        return decoded.get_claims().get_expires_at() > now;
    } catch (...) {
        return false;
    }
}

auto JWTToken::get_subject() const -> std::string {
    try {
        using namespace jwt;
        std::string token_string = header_ + "." + payload_ + "." + signature_;
        auto decoded = decode(token_string);
        return decoded.get_claims().get_subject();
    } catch (...) {
        return "";
    }
}

auto JWTToken::get_expiry() const -> std::chrono::system_clock::time_point {
    try {
        using namespace jwt;
        std::string token_string = header_ + "." + payload_ + "." + signature_;
        auto decoded = decode(token_string);
        return decoded.get_claims().get_expires_at();
    } catch (...) {
        return std::chrono::system_clock::now();
    }
}

auto JWTToken::is_expired() const -> bool {
    return std::chrono::system_clock::now() > get_expiry();
}

auto JWTToken::to_string() const -> std::string {
    return header_ + "." + payload_ + "." + signature_;
}

// Icewall Protection Implementation
auto IcewallProtection::monitor_connection(const std::string& client_ip) -> ThreatLevel {
    // In a real implementation, this would check against Icewall's threat intelligence
    // For now, simulate basic threat detection
    
    // Check if IP is in known bad list (simulated)
    // Uses function-local lazy-initialized heap-allocated set that is never destroyed
    const std::set<std::string>* known_bad_ips_list = get_known_bad_ips();
    if (known_bad_ips_list->find(client_ip) != known_bad_ips_list->end()) {
        return ThreatLevel::HIGH;
    }
    
    // Check for suspicious patterns
    if (client_ip.find("255.255.255") != std::string::npos) {
        return ThreatLevel::MEDIUM;
    }
    
    return ThreatLevel::LOW;
}

auto IcewallProtection::block_ip(const std::string& ip, ThreatLevel level) -> bool {
    // In a real implementation, this would configure firewall rules
    // For now, just log the blocking action
    std::cout << "Icewall: Blocking IP " << ip << " (Threat Level: " << static_cast<int>(level) << ")" << std::endl;
    return true;
}

auto IcewallProtection::log_security_event(const SecurityEvent& event) -> void {
    // In a real implementation, this would log to Icewall's security database
    std::cout << "Icewall Security Event: " << event.description 
              << " (Level: " << static_cast<int>(event.threat_level) 
              << ", IP: " << event.source_ip << ")" << std::endl;
}

auto IcewallProtection::is_ip_blocked(const std::string& ip) -> bool {
    // In a real implementation, this would check firewall rules
    // For now, return false (no IPs blocked in simulation)
    return false;
}

// AES-256 Encryption Implementation
auto AES256Encryption::encrypt(const std::vector<std::byte>& plaintext, const std::vector<std::byte>& key, 
                               const std::vector<std::byte>& iv) -> std::vector<std::byte> {
    // In a real implementation, this would use OpenSSL AES-256
    // For now, simulate encryption with XOR (NOT SECURE - for demonstration only)
    
    if (key.size() < 32) {
        throw std::invalid_argument("Key must be at least 32 bytes for AES-256");
    }
    
    if (iv.size() < 16) {
        throw std::invalid_argument("IV must be at least 16 bytes");
    }
    
    std::vector<std::byte> ciphertext;
    ciphertext.reserve(plaintext.size());
    
    // Simple XOR encryption (NOT SECURE)
    for (size_t i = 0; i < plaintext.size(); ++i) {
        size_t key_index = i % key.size();
        ciphertext.push_back(static_cast<std::byte>(
            static_cast<uint8_t>(plaintext[i]) ^ static_cast<uint8_t>(key[key_index])
        ));
    }
    
    return ciphertext;
}

auto AES256Encryption::decrypt(const std::vector<std::byte>& ciphertext, const std::vector<std::byte>& key, 
                               const std::vector<std::byte>& iv) -> std::vector<std::byte> {
    // XOR decryption is the same as encryption
    return encrypt(ciphertext, key, iv);
}

auto AES256Encryption::generate_key() -> std::vector<std::byte> {
    std::vector<std::byte> key(32); // 256 bits
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);
    
    for (auto& byte : key) {
        byte = static_cast<std::byte>(dis(gen));
    }
    
    return key;
}

auto AES256Encryption::generate_iv() -> std::vector<std::byte> {
    std::vector<std::byte> iv(16); // 128 bits
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);
    
    for (auto& byte : iv) {
        byte = static_cast<std::byte>(dis(gen));
    }
    
    return iv;
}

// TLS Session Implementation
TLSSession::TLSSession(Version version, CipherSuite suite)
    : version_(version), cipher_suite_(suite), is_resumed_(false) {
    // Generate random values for session
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);
    
    client_random_.resize(32);
    server_random_.resize(32);
    
    for (auto& byte : client_random_) {
        byte = static_cast<std::byte>(dis(gen));
    }
    
    for (auto& byte : server_random_) {
        byte = static_cast<std::byte>(dis(gen));
    }
}

auto TLSSession::negotiate_cipher_suite(const std::vector<CipherSuite>& client_suites) -> std::optional<CipherSuite> {
    // Prefer post-quantum suites first
    for (const auto& suite : client_suites) {
        if (suite == CipherSuite::TLS_KYBER768_AES256_GCM_SHA384 ||
            suite == CipherSuite::TLS_DILITHIUM3_AES256_GCM_SHA384) {
            cipher_suite_ = suite;
            return cipher_suite_;
        }
    }
    
    // Fall back to classical suites
    for (const auto& suite : client_suites) {
        if (suite == CipherSuite::TLS_AES_256_GCM_SHA384 ||
            suite == CipherSuite::TLS_AES_128_GCM_SHA256) {
            cipher_suite_ = suite;
            return cipher_suite_;
        }
    }
    
    return std::nullopt;
}

auto TLSSession::generate_master_secret(const std::vector<std::byte>& pre_master_secret) -> void {
    // In a real implementation, this would use proper key derivation
    // For now, just store the pre-master secret
    master_secret_ = pre_master_secret;
}

auto TLSSession::derive_keys() -> std::tuple<std::vector<std::byte>, std::vector<std::byte>, std::vector<std::byte>, std::vector<std::byte>> {
    // In a real implementation, this would derive proper keys using HKDF or PRF
    // For now, generate deterministic keys based on master secret
    
    std::vector<std::byte> client_write_key(32);
    std::vector<std::byte> server_write_key(32);
    std::vector<std::byte> client_write_iv(16);
    std::vector<std::byte> server_write_iv(16);
    
    // Simple key derivation (NOT SECURE - for demonstration only)
    std::hash<std::string> hasher;
    std::string combined;
    
    for (const auto& byte : master_secret_) {
        combined += static_cast<char>(byte);
    }
    
    size_t hash_value = hasher(combined);
    
    // Fill keys with hash-based values
    for (size_t i = 0; i < client_write_key.size(); ++i) {
        client_write_key[i] = static_cast<std::byte>((hash_value >> (i % 32)) & 0xFF);
    }
    
    for (size_t i = 0; i < server_write_key.size(); ++i) {
        server_write_key[i] = static_cast<std::byte>((hash_value >> ((i + 16) % 32)) & 0xFF);
    }
    
    for (size_t i = 0; i < client_write_iv.size(); ++i) {
        client_write_iv[i] = static_cast<std::byte>((hash_value >> ((i + 32) % 32)) & 0xFF);
    }
    
    for (size_t i = 0; i < server_write_iv.size(); ++i) {
        server_write_iv[i] = static_cast<std::byte>((hash_value >> ((i + 48) % 32)) & 0xFF);
    }
    
    return {client_write_key, server_write_key, client_write_iv, server_write_iv};
}

auto TLSSession::is_post_quantum() const -> bool {
    return cipher_suite_ == CipherSuite::TLS_KYBER768_AES256_GCM_SHA384 ||
           cipher_suite_ == CipherSuite::TLS_DILITHIUM3_AES256_GCM_SHA384 ||
           cipher_suite_ == CipherSuite::TLS_KYBER1024_DILITHIUM5_CHACHA20_POLY1305_SHA512;
}

// TLS Handshake Implementation
auto TLSHandshake::create_client_hello(Version max_version, const std::vector<CipherSuite>& supported_suites) -> HandshakeMessage {
    HandshakeMessage msg;
    msg.type = HandshakeType::CLIENT_HELLO;
    msg.timestamp = std::chrono::system_clock::now();
    
    // Serialize client hello data
    std::stringstream ss;
    ss << static_cast<int>(max_version) << "|";
    
    for (size_t i = 0; i < supported_suites.size(); ++i) {
        if (i > 0) ss << ",";
        ss << static_cast<int>(supported_suites[i]);
    }
    
    std::string data = ss.str();
    msg.data.assign(data.begin(), data.end());
    
    return msg;
}

auto TLSHandshake::create_server_hello(Version version, CipherSuite selected_suite, const std::vector<std::byte>& random) -> HandshakeMessage {
    HandshakeMessage msg;
    msg.type = HandshakeType::SERVER_HELLO;
    msg.timestamp = std::chrono::system_clock::now();
    
    // Serialize server hello data
    std::stringstream ss;
    ss << static_cast<int>(version) << "|" << static_cast<int>(selected_suite) << "|";
    
    for (const auto& byte : random) {
        ss << std::hex << std::setfill('0') << std::setw(2) 
           << static_cast<int>(byte) << ":";
    }
    
    std::string data = ss.str();
    msg.data.assign(data.begin(), data.end());
    
    return msg;
}

auto TLSHandshake::create_certificate(const std::vector<std::byte>& cert_chain) -> HandshakeMessage {
    HandshakeMessage msg;
    msg.type = HandshakeType::CERTIFICATE;
    msg.timestamp = std::chrono::system_clock::now();
    msg.data = cert_chain;
    
    return msg;
}

auto TLSHandshake::create_server_key_exchange(const std::vector<std::byte>& params) -> HandshakeMessage {
    HandshakeMessage msg;
    msg.type = HandshakeType::SERVER_KEY_EXCHANGE;
    msg.timestamp = std::chrono::system_clock::now();
    msg.data = params;
    
    return msg;
}

auto TLSHandshake::create_server_hello_done() -> HandshakeMessage {
    HandshakeMessage msg;
    msg.type = HandshakeType::SERVER_HELLO_DONE;
    msg.timestamp = std::chrono::system_clock::now();
    
    return msg;
}

auto TLSHandshake::create_client_key_exchange(const std::vector<std::byte>& encrypted_pre_master_secret) -> HandshakeMessage {
    HandshakeMessage msg;
    msg.type = HandshakeType::CLIENT_KEY_EXCHANGE;
    msg.timestamp = std::chrono::system_clock::now();
    msg.data = encrypted_pre_master_secret;
    
    return msg;
}

auto TLSHandshake::create_certificate_verify(const std::vector<std::byte>& signature) -> HandshakeMessage {
    HandshakeMessage msg;
    msg.type = HandshakeType::CERTIFICATE_VERIFY;
    msg.timestamp = std::chrono::system_clock::now();
    msg.data = signature;
    
    return msg;
}

auto TLSHandshake::create_finished(const std::vector<std::byte>& verify_data) -> HandshakeMessage {
    HandshakeMessage msg;
    msg.type = HandshakeType::FINISHED;
    msg.timestamp = std::chrono::system_clock::now();
    msg.data = verify_data;
    
    return msg;
}

// TLSSecureSocket Implementation
TLSSecureSocket::TLSSecureSocket(const class IPAddress& addr, std::uint16_t port)
    : SecureSocket(addr, port), session_(nullptr), tls_negotiated_(false) {
    // Initialize with preferred cipher suites
    supported_suites_ = {
        CipherSuite::TLS_KYBER768_AES256_GCM_SHA384,
        CipherSuite::TLS_DILITHIUM3_AES256_GCM_SHA384,
        CipherSuite::TLS_AES_256_GCM_SHA384,
        CipherSuite::TLS_AES_128_GCM_SHA256
    };
}

auto TLSSecureSocket::enable_tls(Version min_version, Version max_version) -> bool {
    // In a real implementation, this would configure TLS settings
    // For now, just return true
    return true;
}

auto TLSSecureSocket::perform_handshake() -> bool {
    try {
        // Create TLS session
        session_ = std::make_unique<TLSSession>(Version::TLS_1_3_PQC, CipherSuite::TLS_KYBER768_AES256_GCM_SHA384);
        
        // Generate pre-master secret
        std::vector<std::byte> pre_master_secret = AES256Encryption::generate_key();
        session_->generate_master_secret(pre_master_secret);
        
        tls_negotiated_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

auto TLSSecureSocket::negotiate_post_quantum() -> bool {
    if (!session_) {
        return false;
    }
    
    // Ensure post-quantum cipher suite is selected
    return session_->is_post_quantum();
}

auto TLSSecureSocket::secure_send(secure_span<const std::byte> data) -> std::size_t {
    if (!tls_negotiated_ || !session_) {
        throw std::runtime_error("TLS not negotiated");
    }
    
    // Encrypt data using session keys
    auto keys = session_->derive_keys();
    const auto& client_write_key = std::get<0>(keys);
    const auto& client_write_iv = std::get<2>(keys);
    
    // Encrypt the data
    auto encrypted_data = AES256Encryption::encrypt(
        std::vector<std::byte>(data.begin(), data.end()),
        client_write_key,
        client_write_iv
    );
    
    // In a real implementation, this would send the encrypted data
    // For now, just return the size
    return encrypted_data.size();
}

auto TLSSecureSocket::secure_receive(secure_span<std::byte> buffer) -> std::size_t {
    if (!tls_negotiated_ || !session_) {
        throw std::runtime_error("TLS not negotiated");
    }
    
    // In a real implementation, this would receive and decrypt data
    // For now, just return 0
    return 0;
}

auto TLSSecureSocket::authenticate_with_jwt(const JWTToken& token) -> bool {
    // Validate JWT token
    std::vector<std::byte> dummy_key(32);
    return token.validate(dummy_key);
}

auto TLSSecureSocket::enable_icewall_protection() -> bool {
    // Enable Icewall protection for this connection
    // In a real implementation, this would integrate with Icewall system
    return true;
}

auto TLSSecureSocket::resume_session(const std::vector<std::byte>& session_id) -> bool {
    // Resume TLS session
    // In a real implementation, this would restore session state
    return true;
}

auto TLSSecureSocket::get_session_info() const -> std::optional<std::vector<std::byte>> {
    if (!session_) {
        return std::nullopt;
    }
    
    // Return session identifier
    std::vector<std::byte> session_id(32);
    return session_id;
}

auto TLSSecureSocket::get_negotiated_version() const -> std::optional<Version> {
    if (!session_) {
        return std::nullopt;
    }
    
    return session_->get_version();
}

// TLSContext Implementation
TLSContext::TLSContext(const TLSConfiguration& config)
    : config_(config) {
    // Initialize with default certificates if none provided
}

auto TLSContext::create_secure_socket(const class IPAddress& addr, std::uint16_t port) -> std::unique_ptr<TLSSecureSocket> {
    return std::make_unique<TLSSecureSocket>(addr, port);
}

auto TLSContext::configure_server_certificate(const std::vector<std::byte>& cert, const std::vector<std::byte>& key) -> void {
    server_certificate_ = cert;
    server_private_key_ = key;
}

auto TLSContext::add_client_certificate(const std::string& client_id, const std::vector<std::byte>& cert) -> void {
    client_certificates_[client_id] = cert;
}

auto TLSContext::get_handshake_performance() const -> double {
    // Return simulated handshake performance
    return 1000.0; // 1000 handshakes per second
}

auto TLSContext::get_current_sessions() const -> std::size_t {
    // Return current active sessions
    return 0; // No active sessions in simulation
}

} // namespace dualstack::security::tls