#include "ip_address.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <charconv>

namespace dualstack {

// IPv4 address implementation
ipv4_address::ipv4_address(std::uint32_t addr) : address(addr) {}

auto ipv4_address::to_string() const -> std::string {
    std::ostringstream oss;
    oss << ((address >> 24) & 0xFF) << '.'
        << ((address >> 16) & 0xFF) << '.'
        << ((address >> 8) & 0xFF) << '.'
        << (address & 0xFF);
    return oss.str();
}

auto ipv4_address::from_string(std::string_view str) -> std::expected<ipv4_address, int> {
    std::uint32_t addr = 0;
    int octet = 0;
    int shift = 24;
    
    auto pos = str.find_first_not_of("0123456789.");
    if (pos != std::string_view::npos) {
        return std::unexpected<int>(-1); // Invalid character
    }
    
    pos = 0;
    while (pos < str.length() && shift >= 0) {
        auto dot_pos = str.find('.', pos);
        if (dot_pos == std::string_view::npos) {
            dot_pos = str.length();
        }
        
        std::string octet_str(str.substr(pos, dot_pos - pos));
        if (octet_str.empty() || octet_str.length() > 3) {
            return std::unexpected<int>(-2); // Invalid octet
        }
        
        int value = 0;
        auto result = std::from_chars(octet_str.data(), octet_str.data() + octet_str.length(), value);
        if (result.ec != std::errc{} || value < 0 || value > 255) {
            return std::unexpected<int>(-3); // Invalid octet value
        }
        
        addr |= (static_cast<std::uint32_t>(value) << shift);
        shift -= 8;
        pos = dot_pos + 1;
        
        if (pos > str.length() && shift >= 0) {
            return std::unexpected<int>(-4); // Missing octets
        }
    }
    
    if (shift != -8) {
        return std::unexpected<int>(-5); // Wrong number of octets
    }
    
    return ipv4_address(addr);
}

// IPv6 address implementation
ipv6_address::ipv6_address(std::uint64_t h, std::uint64_t l) : high(h), low(l) {}

auto ipv6_address::to_string() const -> std::string {
    std::ostringstream oss;
    
    // Find longest sequence of zeros for compression
    std::array<std::uint16_t, 8> words{};
    for (std::size_t i = 0; i < 8; ++i) {
        if (i < 4) {
            words[i] = static_cast<std::uint16_t>((high >> (48 - i * 16)) & 0xFFFF);
        } else {
            words[i] = static_cast<std::uint16_t>((low >> (48 - (i - 4) * 16)) & 0xFFFF);
        }
    }
    
    // Find compression range
    std::size_t compress_start = 0;
    std::size_t compress_len = 0;
    std::size_t current_start = 0;
    std::size_t current_len = 0;
    
    for (std::size_t i = 0; i < 8; ++i) {
        if (words[i] == 0) {
            if (current_len == 0) {
                current_start = i;
            }
            ++current_len;
        } else {
            if (current_len > compress_len && current_len > 1) {
                compress_start = current_start;
                compress_len = current_len;
            }
            current_len = 0;
        }
    }
    
    if (current_len > compress_len && current_len > 1) {
        compress_start = current_start;
        compress_len = current_len;
    }
    
    bool compressed = false;
    for (std::size_t i = 0; i < 8; ++i) {
        if (i == compress_start && compress_len > 0) {
            if (i == 0) {
                oss << "::";
            } else {
                oss << ":";
            }
            i += compress_len - 1;
            compressed = true;
        } else {
            if (i > 0 && !compressed) {
                oss << ":";
            }
            oss << std::hex << words[i];
            compressed = false;
        }
    }
    
    if (compress_len == 8) {
        return "::";
    }
    
    return oss.str();
}

auto ipv6_address::from_string(std::string_view str) -> std::expected<ipv6_address, int> {
    if (str.empty()) {
        return std::unexpected<int>(-1);
    }
    
    // Handle special cases
    if (str == "::") {
        return ipv6_address(0, 0);
    }
    
    // Parse IPv6 address
    std::array<std::uint16_t, 8> words{};
    std::size_t word_index = 0;
    std::size_t compress_pos = std::string_view::npos;
    
    auto pos = str.find("::");
    if (pos != std::string_view::npos) {
        compress_pos = pos;
    }
    
    // Parse before ::
    std::size_t parse_pos = 0;
    while (parse_pos < str.length() && word_index < 8) {
        if (parse_pos == compress_pos) {
            // Skip the compressed zeros
            parse_pos += 2;
            if (parse_pos >= str.length()) break;
            
            // Count remaining words
            std::size_t remaining_words = 0;
            std::size_t temp_pos = parse_pos;
            while (temp_pos < str.length()) {
                if (str[temp_pos] == ':') {
                    ++remaining_words;
                }
                ++temp_pos;
            }
            if (str.back() != ':') {
                ++remaining_words;
            }
            
            word_index = 8 - remaining_words;
            continue;
        }
        
        auto colon_pos = str.find(':', parse_pos);
        if (colon_pos == std::string_view::npos) {
            colon_pos = str.length();
        }
        
        std::string word_str(str.substr(parse_pos, colon_pos - parse_pos));
        if (word_str.empty()) {
            return std::unexpected<int>(-2);
        }
        
        // Parse hex word
        std::uint16_t word = 0;
        auto result = std::from_chars(word_str.data(), word_str.data() + word_str.length(), word, 16);
        if (result.ec != std::errc{}) {
            return std::unexpected<int>(-3);
        }
        
        words[word_index++] = word;
        parse_pos = colon_pos + 1;
        
        if (parse_pos > str.length()) break;
    }
    
    // Convert to high/low
    std::uint64_t high = 0;
    std::uint64_t low = 0;
    
    for (std::size_t i = 0; i < 4; ++i) {
        high |= (static_cast<std::uint64_t>(words[i]) << (48 - i * 16));
    }
    for (std::size_t i = 4; i < 8; ++i) {
        low |= (static_cast<std::uint64_t>(words[i]) << (48 - (i - 4) * 16));
    }
    
    return ipv6_address(high, low);
}

// IPAddress implementation
IPAddress::IPAddress(ipv4_address ipv4) : addr_(std::move(ipv4)) {}

IPAddress::IPAddress(ipv6_address ipv6) : addr_(std::move(ipv6)) {}

auto IPAddress::from_string(std::string_view str) -> std::expected<IPAddress, int> {
    // Try IPv6 first (contains ':')
    if (str.find(':') != std::string_view::npos) {
        auto result = ipv6_address::from_string(str);
        if (result.has_value()) {
            return IPAddress(result.value());
        }
        return std::unexpected<int>(result.error());
    }
    
    // Try IPv4
    auto result = ipv4_address::from_string(str);
    if (result.has_value()) {
        return IPAddress(result.value());
    }
    return std::unexpected<int>(result.error());
}

auto IPAddress::to_string() const -> std::string {
    return std::visit([](const auto& addr) { return addr.to_string(); }, addr_);
}

auto IPAddress::get_ipv4() const -> const ipv4_address& {
    return std::get<ipv4_address>(addr_);
}

auto IPAddress::get_ipv6() const -> const ipv6_address& {
    return std::get<ipv6_address>(addr_);
}

bool IPAddress::operator==(const IPAddress& other) const {
    return addr_ == other.addr_;
}

bool IPAddress::operator!=(const IPAddress& other) const {
    return !(*this == other);
}

auto IPAddress::Hash::operator()(const IPAddress& ip) const -> std::size_t {
    return std::hash<std::variant<ipv4_address, ipv6_address>>{}(ip.addr_);
}

} // namespace dualstack