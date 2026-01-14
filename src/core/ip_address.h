#pragma once

#include <variant>
#include <string>
#include <string_view>
#include <expected>
#include <cstdint>

#ifdef __cpp_lib_expected
#include <expected>
#else
// Fallback for older compilers
#include <optional>
namespace std {
    template<typename T, typename E>
    using expected = std::optional<T>;
}
#endif

namespace dualstack {

// Forward declarations
struct ipv4_address;
struct ipv6_address;

class IPAddress {
private:
    std::variant<ipv4_address, ipv6_address> addr_;

public:
    // Constructors
    IPAddress() = default;
    IPAddress(ipv4_address ipv4);
    IPAddress(ipv6_address ipv6);
    
    // Factory methods
    static auto from_string(std::string_view str) -> std::expected<IPAddress, int>;
    auto to_string() const -> std::string;
    
    // Accessors
    bool is_ipv4() const { return std::holds_alternative<ipv4_address>(addr_); }
    bool is_ipv6() const { return std::holds_alternative<ipv6_address>(addr_); }
    
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

// IPv4 address structure
struct ipv4_address {
    std::uint32_t address;
    
    ipv4_address() : address(0) {}
    explicit ipv4_address(std::uint32_t addr) : address(addr) {}
    
    auto to_string() const -> std::string;
    static auto from_string(std::string_view str) -> std::expected<ipv4_address, int>;
    
    bool operator==(const ipv4_address& other) const = default;
};

// IPv6 address structure
struct ipv6_address {
    std::uint64_t high;
    std::uint64_t low;
    
    ipv6_address() : high(0), low(0) {}
    ipv6_address(std::uint64_t h, std::uint64_t l) : high(h), low(l) {}
    
    auto to_string() const -> std::string;
    static auto from_string(std::string_view str) -> std::expected<ipv6_address, int>;
    
    bool operator==(const ipv6_address& other) const = default;
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