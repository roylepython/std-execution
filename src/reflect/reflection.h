#pragma once

#include <type_traits>
#include <concepts>
#include <string_view>
#include <array>

// C++26 reflection support
#if __cpp_lib_meta >= 202207L
#include <meta>
#endif

namespace dualstack::reflect {

// Concepts for reflection-capable types
template<typename T>
concept reflectable = requires(T t) {
    t.reflect(std::declval<auto&>());
};

template<typename T>
concept has_members = requires {
    typename T::member_types;
};

// Compile-time string utilities
template<std::size_t N>
struct fixed_string {
    char data[N];
    
    constexpr fixed_string(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }
    
    constexpr auto operator<=>(const fixed_string&) const = default;
};

// Meta-information extraction
#if __cpp_lib_meta >= 202207L

// Get type name at compile time
template<typename T>
constexpr auto type_name() -> std::string_view {
    constexpr auto info = std::meta::reflect(^T);
    return std::meta::name_of(info);
}

// Get member names and types
template<typename T>
constexpr auto member_names() -> std::array<std::string_view, 0> {
    if constexpr (reflectable<T>) {
        constexpr auto info = std::meta::reflect(^T);
        constexpr auto members = std::meta::members_of(info);
        // This would be implemented with actual reflection in C++26
        return {};
    } else {
        return {};
    }
}

// Generate binding code for IP addresses
template<typename Config>
constexpr auto generate_ip_bindings() {
    constexpr auto config_info = std::meta::reflect(^Config);
    // Generate compile-time code for IP address binding
    return std::meta::members_of(config_info);
}

// Reflection-based serialization
template<reflectable T>
constexpr auto serialize(const T& obj) -> std::string {
    std::string result;
    // This would use reflection to serialize all members
    return result;
}

// Reflection-based deserialization
template<reflectable T>
constexpr auto deserialize(std::string_view data) -> T {
    T obj{};
    // This would use reflection to deserialize into members
    return obj;
}

#endif

// Fallback implementations for older compilers
#ifndef __cpp_lib_meta

template<typename T>
constexpr auto type_name() -> std::string_view {
    return "unknown_type";
}

template<typename T>
constexpr auto member_names() -> std::array<std::string_view, 0> {
    return {};
}

#endif

// Utility macros for reflection
#define DUALSTACK_REFLECT_MEMBER(member) \
    r.reflect(member, #member)

#define DUALSTACK_REFLECT_TYPE(Type) \
    template<typename Reflection> \
    constexpr void reflect(Type& obj, Reflection& r) { \
        obj.reflect(r); \
    }

// Compile-time configuration generation
template<typename ConfigType>
constexpr auto generate_config_bindings() {
    if constexpr (reflectable<ConfigType>) {
        return generate_ip_bindings<ConfigType>();
    } else {
        return std::array<int, 0>{};
    }
}

// Type traits for networking types
template<typename T>
struct is_network_type : std::false_type {};

template<>
struct is_network_type<class IPAddress> : std::true_type {};

template<>
struct is_network_type<class Socket> : std::true_type {};

template<>
struct is_network_type<class Acceptor> : std::true_type {};

template<typename T>
constexpr bool is_network_type_v = is_network_type<T>::value;

} // namespace dualstack::reflect