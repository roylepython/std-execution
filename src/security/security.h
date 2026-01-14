#pragma once

// Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved

#include <span>
#include <concepts>
#include <contract>
#include <memory>

// C++26 hardened library support
#if __cpp_lib_hardened >= 202300L
#include <hardened>
#endif

namespace dualstack::security {

// Security-enhanced span with bounds checking
#if __cpp_lib_hardened >= 202300L
template<typename T>
using secure_span = std::span<T, std::hardened>;
#else
template<typename T>
using secure_span = std::span<T>;
#endif

// Contract checking utilities
#if __cpp_lib_contracts >= 202200L

// Precondition contracts
#define DUALSTACK_EXPECTS(condition) [[expects: condition]]
#define DUALSTACK_ENSURES(condition) [[ensures: condition]]

// Auditing contracts
#define DUALSTACK_AUDIT(condition) [[audit: condition]]

#else

// Fallback implementations
#define DUALSTACK_EXPECTS(condition) static_assert(condition)
#define DUALSTACK_ENSURES(condition) static_assert(condition)
#define DUALSTACK_AUDIT(condition) 

#endif

// Secure socket wrapper
class SecureSocket {
private:
    class impl;
    std::unique_ptr<impl> impl_;
    
public:
    // Construction with security constraints
    DUALSTACK_EXPECTS(port > 0 && port < 65536)
    DUALSTACK_EXPECTS(!addr.is_reserved())
    SecureSocket(const class IPAddress& addr, std::uint16_t port);
    
    ~SecureSocket();
    
    // Secure data transfer with bounds checking
    DUALSTACK_EXPECTS(!data.empty())
    DUALSTACK_ENSURES(ret <= data.size())
    auto secure_send(secure_span<const std::byte> data) -> std::size_t;
    
    DUALSTACK_EXPECTS(!buffer.empty())
    DUALSTACK_ENSURES(ret <= buffer.size())
    auto secure_receive(secure_span<std::byte> buffer) -> std::size_t;
    
    // Connection security
    auto enable_encryption() -> bool;
    auto verify_peer_certificate() -> bool;
    auto get_security_level() const -> int;
};

// Access Control List with bounds checking
class AccessControlList {
private:
    secure_span<const class IPAddress> blocked_ips_;
    
public:
    explicit AccessControlList(secure_span<const class IPAddress> blocked);
    
    DUALSTACK_EXPECTS(!ip.is_unspecified())
    auto is_blocked(const class IPAddress& ip) const -> bool;
    
    auto add_blocked(const class IPAddress& ip) -> void;
    auto remove_blocked(const class IPAddress& ip) -> void;
    auto get_blocked_count() const -> std::size_t;
};

// Buffer overflow protection
template<typename T, std::size_t MaxSize>
class SecureBuffer {
private:
    std::array<T, MaxSize> buffer_;
    std::size_t size_;
    
public:
    SecureBuffer() : size_(0) {}
    
    DUALSTACK_EXPECTS(index < size_)
    auto at(std::size_t index) const -> const T& {
        return buffer_[index];
    }
    
    DUALSTACK_EXPECTS(index < size_)
    auto at(std::size_t index) -> T& {
        return buffer_[index];
    }
    
    DUALSTACK_EXPECTS(new_size <= MaxSize)
    auto resize(std::size_t new_size) -> void {
        size_ = new_size;
    }
    
    auto size() const -> std::size_t { return size_; }
    auto max_size() const -> std::size_t { return MaxSize; }
    auto empty() const -> bool { return size_ == 0; }
    
    auto begin() -> T* { return buffer_.data(); }
    auto end() -> T* { return buffer_.data() + size_; }
    auto begin() const -> const T* { return buffer_.data(); }
    auto end() const -> const T* { return buffer_.data() + size_; }
};

// Zero-memory cleanup utilities
template<typename T>
auto secure_zero_memory(T* ptr, std::size_t count) -> void {
    // Platform-specific secure zeroing
    #ifdef _WIN32
        SecureZeroMemory(ptr, count * sizeof(T));
    #else
        explicit_bzero(ptr, count * sizeof(T));
    #endif
}

template<typename Container>
auto secure_clear(Container& container) -> void {
    if (!container.empty()) {
        secure_zero_memory(container.data(), container.size());
        container.clear();
    }
}

// Cryptographic hash utilities
class HashValidator {
public:
    enum class Algorithm {
        SHA256,
        SHA512,
        MD5  // Deprecated but sometimes needed
    };
    
    static auto hash(secure_span<const std::byte> data, Algorithm alg = Algorithm::SHA256) 
        -> std::array<std::byte, 64>;
        
    static auto verify(secure_span<const std::byte> data, 
                      secure_span<const std::byte> hash,
                      Algorithm alg = Algorithm::SHA256) -> bool;
};

// Security audit logging
class SecurityAudit {
public:
    enum class Level {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    
    static auto log(Level level, std::string_view message) -> void;
    static auto is_audit_enabled() -> bool;
    static auto enable_audit(bool enable) -> void;
};

// Type traits for security validation
template<typename T>
concept secure_type = requires(T t) {
    t.secure_send(std::declval<secure_span<const std::byte>>());
    t.secure_receive(std::declval<secure_span<std::byte>>());
};

// Compile-time security checks
template<typename T>
constexpr bool has_security_features() {
    if constexpr (requires { typename T::secure_tag; }) {
        return true;
    } else {
        return false;
    }
}

} // namespace dualstack::security