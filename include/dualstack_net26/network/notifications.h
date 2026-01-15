/**
 * Amphisbaena 🐍 - Notification System
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * PUBLIC LIBRARY - Notification System with Lamia Backend Support
 * 
 * Comprehensive notification system for MedusaServ Notification Server and Purple Pages
 * Supports C++ and Lamia backend integration
 * 
 * Yorkshire Champion Standards - Improving AI Safety and the Web
 */

#pragma once

// Include format header fix BEFORE any standard headers to prevent GCC 14.2.0 format header bug
#include "../fix_format_header.h"
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <cstdint>

// Public API Export for DLL/SO
#ifdef AMPHISBAENA_BUILDING_LIBRARY
    #ifdef _WIN32
        #define AMPHISBAENA_API __declspec(dllexport)
    #else
        #define AMPHISBAENA_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef _WIN32
        #define AMPHISBAENA_API __declspec(dllimport)
    #else
        #define AMPHISBAENA_API __attribute__((visibility("default")))
    #endif
#endif

// C interface for Lamia backend integration
extern "C" {
    // Forward declarations for C interface
    struct NotificationContext;
    typedef struct NotificationContext* NotificationContextHandle;
    
    // C interface functions (implemented in .cpp)
    AMPHISBAENA_API NotificationContextHandle notification_context_create();
    AMPHISBAENA_API void notification_context_destroy(NotificationContextHandle ctx);
    AMPHISBAENA_API int notification_send(NotificationContextHandle ctx,
                                         const char* source_id,
                                         const char* source_component,
                                         uint16_t category,
                                         uint8_t severity,
                                         const char* title,
                                         const char* message);
}

namespace dualstack {
namespace network {
namespace notifications {

/**
 * @brief Notification Severity Levels
 */
enum class Severity : uint8_t {
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    CRITICAL = 3,
    DEBUG = 4
};

/**
 * @brief Notification Categories
 */
enum class Category : uint16_t {
    SESSION = 0x0001,
    USER = 0x0002,
    CDN = 0x0003,
    SYSTEM = 0x0004,
    SECURITY = 0x0005,
    PERFORMANCE = 0x0006,
    CONFIGURATION = 0x0007,
    HEALTH = 0x0008,
    ERROR_REPORT = 0x0009,
    WARNING_REPORT = 0x000A
};

/**
 * @brief Notification Structure
 * 
 * Complete notification with metadata for routing and processing
 */
struct AMPHISBAENA_API Notification {
    // Identification
    std::string notification_id;
    std::string source_id;          // Source system (e.g., "psiforcedb", "medusaserv", "galaxycdn")
    std::string source_component;    // Component within source (e.g., "session_manager", "auth")
    
    // Classification
    Category category;
    Severity severity;
    
    // Content
    std::string title;
    std::string message;
    std::string detailed_message;   // Extended details for errors/warnings
    
    // Context
    std::string session_id;         // Session ID if applicable
    std::string user_id;            // User ID if applicable
    std::string connection_id;      // Connection ID if applicable
    
    // Metadata
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> metadata;  // Additional key-value pairs
    
    // Error/Warning specific fields (for Purple Pages)
    std::string error_code;         // Error code if applicable
    std::string error_type;         // Error type classification
    std::string resolution_hint;    // Suggested resolution
    std::vector<std::string> affected_components;  // Components affected by error/warning
    
    Notification()
        : category(Category::SYSTEM)
        , severity(Severity::INFO)
        , timestamp(std::chrono::system_clock::now())
    {
        // Generate unique notification ID
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        notification_id = "notif_" + std::to_string(millis) + "_" + std::to_string(reinterpret_cast<uintptr_t>(this));
    }
};

/**
 * @brief Notification Handler Interface
 * 
 * Interface for handling notifications (C++ callback)
 */
class AMPHISBAENA_API INotificationHandler {
public:
    virtual ~INotificationHandler() = default;
    virtual void handle_notification(const Notification& notification) = 0;
};

/**
 * @brief Notification Manager
 * 
 * Central notification management system with Lamia backend support
 */
class AMPHISBAENA_API NotificationManager {
public:
    NotificationManager();
    ~NotificationManager();

    // Initialization
    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }

    // Registration
    void register_handler(std::shared_ptr<INotificationHandler> handler);
    void register_callback(std::function<void(const Notification&)> callback);
    
    // Lamia backend integration
    void register_lamia_handler(NotificationContextHandle lamia_context);
    bool is_lamia_enabled() const { return lamia_context_ != nullptr; }

    // Notification sending
    void send_notification(const Notification& notification);
    void send_session_event(const std::string& session_id, const std::string& event_type, 
                           const std::string& message, Severity severity = Severity::INFO);
    void send_user_event(const std::string& user_id, const std::string& event_type,
                        const std::string& message, Severity severity = Severity::INFO);
    void send_cdn_event(const std::string& event_type, const std::string& message,
                       Severity severity = Severity::INFO);
    void send_error(const std::string& source_component, const std::string& error_code,
                   const std::string& error_message, const std::string& detailed_message = "",
                   const std::string& resolution_hint = "");
    void send_warning(const std::string& source_component, const std::string& warning_code,
                    const std::string& warning_message, const std::string& detailed_message = "",
                    const std::string& resolution_hint = "");

    // Statistics
    size_t get_notification_count() const { return notification_count_.load(); }
    size_t get_error_count() const { return error_count_.load(); }
    size_t get_warning_count() const { return warning_count_.load(); }

    // Configuration
    void set_notification_server_endpoint(const std::string& host, uint16_t port);
    void enable_lamia_backend(bool enable);

private:
    std::atomic<bool> initialized_;
    std::mutex handlers_mutex_;
    std::vector<std::shared_ptr<INotificationHandler>> handlers_;
    std::vector<std::function<void(const Notification&)>> callbacks_;
    
    // Lamia backend
    NotificationContextHandle lamia_context_;
    std::atomic<bool> lamia_enabled_{false};
    
    // Notification server endpoint
    std::string notification_server_host_;
    uint16_t notification_server_port_;
    std::mutex endpoint_mutex_;
    
    // Statistics
    std::atomic<size_t> notification_count_{0};
    std::atomic<size_t> error_count_{0};
    std::atomic<size_t> warning_count_{0};
    
    // Internal processing
    void process_notification(const Notification& notification);
    void send_to_lamia_backend(const Notification& notification);
    void send_to_notification_server(const Notification& notification);
};

/**
 * @brief Default Notification Handler
 * 
 * Default implementation that logs notifications
 */
class AMPHISBAENA_API DefaultNotificationHandler : public INotificationHandler {
public:
    void handle_notification(const Notification& notification) override;
};

} // namespace notifications
} // namespace network
} // namespace dualstack

// C interface for Lamia backend
extern "C" {
    /**
     * @brief Create notification context for Lamia
     */
    AMPHISBAENA_API NotificationContextHandle notification_context_create();
    
    /**
     * @brief Destroy notification context
     */
    AMPHISBAENA_API void notification_context_destroy(NotificationContextHandle ctx);
    
    /**
     * @brief Send notification from Lamia
     */
    AMPHISBAENA_API int notification_send(NotificationContextHandle ctx,
                                         const char* source_id,
                                         const char* source_component,
                                         uint16_t category,
                                         uint8_t severity,
                                         const char* title,
                                         const char* message);
}

