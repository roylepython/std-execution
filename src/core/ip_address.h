#pragma once

// Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved

// Include format header fix BEFORE any standard headers
#include "../../include/dualstack_net26/fix_format_header.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <optional>
#include <expected>
#include <functional>

// Use standard library expected for C++23+
namespace dualstack_expected = std;

// Helper function to create unexpected values
template<typename E>
inline auto make_unexpected_value(E&& err) -> std::unexpected<std::decay_t<E>> {
    return std::unexpected<std::decay_t<E>>(std::forward<E>(err));
}

namespace dualstack {

// IPv4 address structure (defined before IPAddress to avoid incomplete type issues)
struct ipv4_address {
    std::uint32_t address;
    
    ipv4_address() : address(0) {}
    explicit ipv4_address(std::uint32_t addr) : address(addr) {}
    
    auto to_string() const -> std::string;
    static auto from_string(std::string_view str) -> dualstack_expected::expected<ipv4_address, int>;
    
    bool operator==(const ipv4_address& other) const = default;
    auto operator<=>(const ipv4_address& other) const = default;
};

// IPv6 address structure (defined before IPAddress to avoid incomplete type issues)
struct ipv6_address {
    std::uint64_t high;
    std::uint64_t low;
    
    ipv6_address() : high(0), low(0) {}
    ipv6_address(std::uint64_t h, std::uint64_t l) : high(h), low(l) {}
    
    auto to_string() const -> std::string;
    static auto from_string(std::string_view str) -> dualstack_expected::expected<ipv6_address, int>;
    
    bool operator==(const ipv6_address& other) const = default;
    auto operator<=>(const ipv6_address& other) const = default;
};

class IPAddress {
private:
    std::variant<ipv4_address, ipv6_address> addr_;

public:
    // Constructors
    IPAddress() = default;
    IPAddress(ipv4_address ipv4);
    IPAddress(ipv6_address ipv6);
    
    // Factory methods
    static auto from_string(std::string_view str) -> dualstack_expected::expected<IPAddress, int>;
    auto to_string() const -> std::string;
    
    // Accessors
    bool is_ipv4() const { 
        return std::holds_alternative<ipv4_address>(addr_); 
    }
    bool is_ipv6() const { 
        return std::holds_alternative<ipv6_address>(addr_); 
    }
    
    auto get_ipv4() const -> const ipv4_address&;
    auto get_ipv6() const -> const ipv6_address&;
    
    // Comparison operators
    bool operator==(const IPAddress& other) const;
    bool operator!=(const IPAddress& other) const;
    
    // Hash support for use in containers
    struct Hash {
        auto operator()(const IPAddress& ip) const -> std::size_t;
    };
};

// Compile-time reflection support (C++26)
#if __cpp_lib_meta >= 202207L
template<typename Reflection>
constexpr void reflect(IPAddress& obj, Reflection& r) {
    r.reflect(obj.addr_, "address");
}

template<typename Reflection>
constexpr void reflect(ipv4_address& obj, Reflection& r) {
    r.reflect(obj.address, "address");
}

template<typename Reflection>
constexpr void reflect(ipv6_address& obj, Reflection& r) {
    r.reflect(obj.high, "high");
    r.reflect(obj.low, "low");
}
#endif

} // namespace dualstack