#pragma once

// Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved

#include "security.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

// TLS Protocol Implementation with PQC support
namespace dualstack::security::tls {

// TLS Version enumeration
enum class Version {
    SSL_3_0 = 0x0300,
    TLS_1_0 = 0x0301,
    TLS_1_1 = 0x0302,
    TLS_1_2 = 0x0303,
    TLS_1_3 = 0x0304,
    TLS_1_3_PQC = 0x0305  // Post-Quantum variant
};

// Cipher suites supporting both classical and post-quantum cryptography
enum class CipherSuite {
    // Classical TLS 1.3 suites
    TLS_AES_128_GCM_SHA256 = 0x1301,
    TLS_AES_256_GCM_SHA384 = 0x1302,
    TLS_CHACHA20_POLY1305_SHA256 = 0x1303,
    
    // Post-Quantum suites (PsiForceDB integration)
    TLS_KYBER768_AES256_GCM_SHA384 = 0x1304,
    TLS_DILITHIUM3_AES256_GCM_SHA384 = 0x1305,
    TLS_KYBER1024_DILITHIUM5_CHACHA20_POLY1305_SHA512 = 0x1306,
    
    // Backward compatibility suites
    TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 = 0xC02F,
    TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 = 0xC030,
    TLS_RSA_WITH_AES_128_GCM_SHA256 = 0x009C,
    TLS_RSA_WITH_AES_256_GCM_SHA384 = 0x009D
};

// Post-Quantum Cryptography integration with PsiForceDB
namespace pqc {
    
    // Kyber lattice-based key encapsulation
    class KyberKeyExchange {
    public:
        static auto generate_keypair() -> std::pair<std::vector<std::byte>, std::vector<std::byte>>;
        static auto encapsulate(const std::vector<std::byte>& public_key) -> std::pair<std::vector<std::byte>, std::vector<std::byte>>;
        static auto decapsulate(const std::vector<std::byte>& ciphertext, const std::vector<std::byte>& private_key) -> std::vector<std::byte>;
    };
    
    // Dilithium lattice-based signatures
    class DilithiumSignature {
    public:
        static auto generate_keypair() -> std::pair<std::vector<std::byte>, std::vector<std::byte>>;
        static auto sign(const std::vector<std::byte>& message, const std::vector<std::byte>& private_key) -> std::vector<std::byte>;
        static auto verify(const std::vector<std::byte>& message, const std::vector<std::byte>& signature, const std::vector<std::byte>& public_key) -> bool;
    };
    
    // Hybrid PQC + Classical cryptography
    class HybridCrypto {
    public:
        static auto combine_keys(const std::vector<std::byte>& pqc_key, const std::vector<std::byte>& classical_key) -> std::vector<std::byte>;
        static auto split_keys(const std::vector<std::byte>& combined_key) -> std::pair<std::vector<std::byte>, std::vector<std::byte>>;
    };
}

// JWT Token handling for authentication
class JWTToken {
private:
    std::string header_;
    std::string payload_;
    std::string signature_;
    
public:
    JWTToken() = default;
    JWTToken(const std::string& token);
    
    static auto create(const std::string& subject, const std::chrono::seconds& expiry, 
                      const std::map<std::string, std::string>& claims) -> JWTToken;
    
    auto validate(const std::vector<std::byte>& public_key) const -> bool;
    auto get_subject() const -> std::string;
    auto get_expiry() const -> std::chrono::system_clock::time_point;
    auto is_expired() const -> bool;
    auto to_string() const -> std::string;
};

// Icewall Security System integration
class IcewallProtection {
public:
    enum class ThreatLevel {
        LOW = 1,
        MEDIUM = 2,
        HIGH = 3,
        CRITICAL = 4
    };
    
    struct SecurityEvent {
        ThreatLevel threat_level;
        std::string description;
        std::chrono::system_clock::time_point timestamp;
        std::string source_ip;
    };
    
    static auto monitor_connection(const std::string& client_ip) -> ThreatLevel;
    static auto block_ip(const std::string& ip, ThreatLevel level) -> bool;
    static auto log_security_event(const SecurityEvent& event) -> void;
    static auto is_ip_blocked(const std::string& ip) -> bool;
};

// AES-256 encryption utilities
class AES256Encryption {
public:
    static auto encrypt(const std::vector<std::byte>& plaintext, const std::vector<std::byte>& key, 
                       const std::vector<std::byte>& iv) -> std::vector<std::byte>;
    
    static auto decrypt(const std::vector<std::byte>& ciphertext, const std::vector<std::byte>& key, 
                       const std::vector<std::byte>& iv) -> std::vector<std::byte>;
    
    static auto generate_key() -> std::vector<std::byte>;
    static auto generate_iv() -> std::vector<std::byte>;
};

// TLS Session management
class TLSSession {
private:
    Version version_;
    CipherSuite cipher_suite_;
    std::vector<std::byte> master_secret_;
    std::vector<std::byte> client_random_;
    std::vector<std::byte> server_random_;
    bool is_resumed_;
    
public:
    TLSSession(Version version, CipherSuite suite);
    
    auto negotiate_cipher_suite(const std::vector<CipherSuite>& client_suites) -> std::optional<CipherSuite>;
    auto generate_master_secret(const std::vector<std::byte>& pre_master_secret) -> void;
    auto derive_keys() -> std::tuple<std::vector<std::byte>, std::vector<std::byte>, std::vector<std::byte>, std::vector<std::byte>>;
    
    auto get_version() const -> Version { return version_; }
    auto get_cipher_suite() const -> CipherSuite { return cipher_suite_; }
    auto is_post_quantum() const -> bool;
};

// TLS Handshake protocol
class TLSHandshake {
public:
    enum class HandshakeType {
        CLIENT_HELLO = 1,
        SERVER_HELLO = 2,
        CERTIFICATE = 11,
        SERVER_KEY_EXCHANGE = 12,
        CERTIFICATE_REQUEST = 13,
        SERVER_HELLO_DONE = 14,
        CERTIFICATE_VERIFY = 15,
        CLIENT_KEY_EXCHANGE = 16,
        FINISHED = 20
    };
    
    struct HandshakeMessage {
        HandshakeType type;
        std::vector<std::byte> data;
        std::chrono::system_clock::time_point timestamp;
    };
    
    static auto create_client_hello(Version max_version, const std::vector<CipherSuite>& supported_suites) -> HandshakeMessage;
    static auto create_server_hello(Version version, CipherSuite selected_suite, const std::vector<std::byte>& random) -> HandshakeMessage;
    static auto create_certificate(const std::vector<std::byte>& cert_chain) -> HandshakeMessage;
    static auto create_server_key_exchange(const std::vector<std::byte>& params) -> HandshakeMessage;
    static auto create_server_hello_done() -> HandshakeMessage;
    static auto create_client_key_exchange(const std::vector<std::byte>& encrypted_pre_master_secret) -> HandshakeMessage;
    static auto create_certificate_verify(const std::vector<std::byte>& signature) -> HandshakeMessage;
    static auto create_finished(const std::vector<std::byte>& verify_data) -> HandshakeMessage;
};

// Enhanced Secure Socket with TLS support
class TLSSecureSocket : public SecureSocket {
private:
    std::unique_ptr<TLSSession> session_;
    std::vector<CipherSuite> supported_suites_;
    bool tls_negotiated_;
    
public:
    TLSSecureSocket(const class IPAddress& addr, std::uint16_t port);
    
    // TLS-specific methods
    auto enable_tls(Version min_version = Version::TLS_1_2, 
                   Version max_version = Version::TLS_1_3_PQC) -> bool;
    
    auto perform_handshake() -> bool;
    auto negotiate_post_quantum() -> bool;
    
    // Override secure send/receive with TLS encryption
    auto secure_send(secure_span<const std::byte> data) -> std::size_t override;
    auto secure_receive(secure_span<std::byte> buffer) -> std::size_t override;
    
    // JWT authentication
    auto authenticate_with_jwt(const JWTToken& token) -> bool;
    
    // Icewall protection
    auto enable_icewall_protection() -> bool;
    
    // Session resumption
    auto resume_session(const std::vector<std::byte>& session_id) -> bool;
    auto get_session_info() const -> std::optional<std::vector<std::byte>>;
    
    auto is_tls_negotiated() const -> bool { return tls_negotiated_; }
    auto get_negotiated_version() const -> std::optional<Version>;
};

// TLS Configuration for PsiForceDB integration
struct TLSConfiguration {
    Version min_version = Version::TLS_1_2;
    Version max_version = Version::TLS_1_3_PQC;
    std::vector<CipherSuite> preferred_suites = {
        CipherSuite::TLS_KYBER768_AES256_GCM_SHA384,
        CipherSuite::TLS_DILITHIUM3_AES256_GCM_SHA384,
        CipherSuite::TLS_AES_256_GCM_SHA384,
        CipherSuite::TLS_AES_128_GCM_SHA256
    };
    bool require_pqc = false;
    bool enable_icewall = true;
    std::chrono::minutes session_timeout = std::chrono::minutes(30);
    
    template<typename Reflection>
    constexpr void reflect(Reflection& r) {
        r.reflect(min_version, "min_version");
        r.reflect(max_version, "max_version");
        r.reflect(preferred_suites, "preferred_suites");
        r.reflect(require_pqc, "require_pqc");
        r.reflect(enable_icewall, "enable_icewall");
        r.reflect(session_timeout, "session_timeout");
    }
};

// TLS Context for managing multiple secure connections
class TLSContext {
private:
    TLSConfiguration config_;
    std::vector<std::byte> server_certificate_;
    std::vector<std::byte> server_private_key_;
    std::map<std::string, std::vector<std::byte>> client_certificates_;
    
public:
    explicit TLSContext(const TLSConfiguration& config = {});
    
    auto create_secure_socket(const class IPAddress& addr, std::uint16_t port) -> std::unique_ptr<TLSSecureSocket>;
    auto configure_server_certificate(const std::vector<std::byte>& cert, const std::vector<std::byte>& key) -> void;
    auto add_client_certificate(const std::string& client_id, const std::vector<std::byte>& cert) -> void;
    
    auto get_configuration() const -> const TLSConfiguration& { return config_; }
    auto set_configuration(const TLSConfiguration& config) -> void { config_ = config; }
    
    // Performance monitoring
    auto get_handshake_performance() const -> double; // handshakes per second
    auto get_current_sessions() const -> std::size_t;
};

} // namespace dualstack::security::tls