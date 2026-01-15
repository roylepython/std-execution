/**
 * Amphisbaena 🐍 - Notification System Implementation
 * Copyright © 2025 D Hargreaves | Roylepython AKA The Medusa Initiative 2025 - All Rights Reserved
 * 
 * PUBLIC LIBRARY IMPLEMENTATION
 */

// Include format header fix BEFORE any standard headers
#include "../../include/dualstack_net26/fix_format_header.h"
#include "../../include/dualstack_net26/network/notifications.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>

namespace dualstack {
namespace network {
namespace notifications {

// ============================================================================
// DefaultNotificationHandler Implementation
// ============================================================================

void DefaultNotificationHandler::handle_notification(const Notification& notification) {
    // Format timestamp
    auto time_t = std::chrono::system_clock::to_time_t(notification.timestamp);
    std::tm tm_buf;
    #ifdef _WIN32
    localtime_s(&tm_buf, &time_t);
    #else
    localtime_r(&time_t, &tm_buf);
    #endif
    
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    
    // Severity string
    const char* severity_str = "INFO";
    switch (notification.severity) {
        case Severity::WARNING: severity_str = "WARNING"; break;
        case Severity::ERROR: severity_str = "ERROR"; break;
        case Severity::CRITICAL: severity_str = "CRITICAL"; break;
        case Severity::DEBUG: severity_str = "DEBUG"; break;
        default: break;
    }
    
    // Category string
    const char* category_str = "SYSTEM";
    switch (notification.category) {
        case Category::SESSION: category_str = "SESSION"; break;
        case Category::USER: category_str = "USER"; break;
        case Category::CDN: category_str = "CDN"; break;
        case Category::SECURITY: category_str = "SECURITY"; break;
        case Category::PERFORMANCE: category_str = "PERFORMANCE"; break;
        case Category::CONFIGURATION: category_str = "CONFIG"; break;
        case Category::HEALTH: category_str = "HEALTH"; break;
        case Category::ERROR_REPORT: category_str = "ERROR_REPORT"; break;
        case Category::WARNING_REPORT: category_str = "WARNING_REPORT"; break;
        default: break;
    }
    
    std::cout << "[" << ss.str() << "] [" << severity_str << "] [" << category_str << "] "
              << "[" << notification.source_id << "::" << notification.source_component << "] "
              << notification.title << ": " << notification.message << std::endl;
    
    if (!notification.detailed_message.empty()) {
        std::cout << "  Details: " << notification.detailed_message << std::endl;
    }
    
    if (!notification.error_code.empty()) {
        std::cout << "  Error Code: " << notification.error_code << std::endl;
    }
    
    if (!notification.resolution_hint.empty()) {
        std::cout << "  Resolution: " << notification.resolution_hint << std::endl;
    }
}

// ============================================================================
// NotificationManager Implementation
// ============================================================================

NotificationManager::NotificationManager()
    : initialized_(false)
    , lamia_context_(nullptr)
    , notification_server_port_(0)
{
}

NotificationManager::~NotificationManager() {
    shutdown();
}

bool NotificationManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        // Initialize default handler
        register_handler(std::make_shared<DefaultNotificationHandler>());
        
        initialized_ = true;
        std::cout << "🐍 NotificationManager initialized" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ NotificationManager initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void NotificationManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_.clear();
        callbacks_.clear();
    }
    
    if (lamia_context_) {
        notification_context_destroy(lamia_context_);
        lamia_context_ = nullptr;
    }
    
    initialized_ = false;
    std::cout << "🐍 NotificationManager shutdown" << std::endl;
}

void NotificationManager::register_handler(std::shared_ptr<INotificationHandler> handler) {
    if (!handler) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_.push_back(handler);
}

void NotificationManager::register_callback(std::function<void(const Notification&)> callback) {
    if (!callback) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    callbacks_.push_back(callback);
}

void NotificationManager::register_lamia_handler(NotificationContextHandle lamia_context) {
    if (lamia_context_) {
        notification_context_destroy(lamia_context_);
    }
    lamia_context_ = lamia_context;
    lamia_enabled_ = (lamia_context_ != nullptr);
}

void NotificationManager::send_notification(const Notification& notification) {
    if (!initialized_) {
        return;
    }
    
    process_notification(notification);
}

void NotificationManager::send_session_event(const std::string& session_id, const std::string& event_type,
                                            const std::string& message, Severity severity) {
    Notification notification;
    notification.source_id = "psiforcedb";
    notification.source_component = "session_manager";
    notification.category = Category::SESSION;
    notification.severity = severity;
    notification.session_id = session_id;
    notification.title = "Session Event: " + event_type;
    notification.message = message;
    notification.timestamp = std::chrono::system_clock::now();
    notification.metadata["event_type"] = event_type;
    
    send_notification(notification);
}

void NotificationManager::send_user_event(const std::string& user_id, const std::string& event_type,
                                         const std::string& message, Severity severity) {
    Notification notification;
    notification.source_id = "psiforcedb";
    notification.source_component = "user_manager";
    notification.category = Category::USER;
    notification.severity = severity;
    notification.user_id = user_id;
    notification.title = "User Event: " + event_type;
    notification.message = message;
    notification.timestamp = std::chrono::system_clock::now();
    notification.metadata["event_type"] = event_type;
    
    send_notification(notification);
}

void NotificationManager::send_cdn_event(const std::string& event_type, const std::string& message,
                                        Severity severity) {
    Notification notification;
    notification.source_id = "galaxycdn";
    notification.source_component = "cdn_manager";
    notification.category = Category::CDN;
    notification.severity = severity;
    notification.title = "CDN Event: " + event_type;
    notification.message = message;
    notification.timestamp = std::chrono::system_clock::now();
    notification.metadata["event_type"] = event_type;
    
    send_notification(notification);
}

void NotificationManager::send_error(const std::string& source_component, const std::string& error_code,
                                    const std::string& error_message, const std::string& detailed_message,
                                    const std::string& resolution_hint) {
    Notification notification;
    notification.source_id = "psiforcedb";
    notification.source_component = source_component;
    notification.category = Category::ERROR_REPORT;
    notification.severity = Severity::ERROR;
    notification.title = "Error: " + error_code;
    notification.message = error_message;
    notification.detailed_message = detailed_message;
    notification.error_code = error_code;
    notification.resolution_hint = resolution_hint;
    notification.timestamp = std::chrono::system_clock::now();
    
    // Classify error type
    if (error_code.find("NETWORK") != std::string::npos) {
        notification.error_type = "NETWORK";
    } else if (error_code.find("AUTH") != std::string::npos) {
        notification.error_type = "AUTHENTICATION";
    } else if (error_code.find("DB") != std::string::npos) {
        notification.error_type = "DATABASE";
    } else if (error_code.find("CONFIG") != std::string::npos) {
        notification.error_type = "CONFIGURATION";
    } else {
        notification.error_type = "GENERAL";
    }
    
    error_count_.fetch_add(1);
    send_notification(notification);
}

void NotificationManager::send_warning(const std::string& source_component, const std::string& warning_code,
                                      const std::string& warning_message, const std::string& detailed_message,
                                      const std::string& resolution_hint) {
    Notification notification;
    notification.source_id = "psiforcedb";
    notification.source_component = source_component;
    notification.category = Category::WARNING_REPORT;
    notification.severity = Severity::WARNING;
    notification.title = "Warning: " + warning_code;
    notification.message = warning_message;
    notification.detailed_message = detailed_message;
    notification.error_code = warning_code;  // Reuse field for warning code
    notification.resolution_hint = resolution_hint;
    notification.timestamp = std::chrono::system_clock::now();
    
    // Classify warning type
    if (warning_code.find("PERF") != std::string::npos) {
        notification.error_type = "PERFORMANCE";
    } else if (warning_code.find("DEPRECATED") != std::string::npos) {
        notification.error_type = "DEPRECATION";
    } else if (warning_code.find("CONFIG") != std::string::npos) {
        notification.error_type = "CONFIGURATION";
    } else {
        notification.error_type = "GENERAL";
    }
    
    warning_count_.fetch_add(1);
    send_notification(notification);
}

void NotificationManager::set_notification_server_endpoint(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(endpoint_mutex_);
    notification_server_host_ = host;
    notification_server_port_ = port;
}

void NotificationManager::enable_lamia_backend(bool enable) {
    if (enable && !lamia_context_) {
        lamia_context_ = notification_context_create();
        lamia_enabled_ = (lamia_context_ != nullptr);
    } else if (!enable && lamia_context_) {
        notification_context_destroy(lamia_context_);
        lamia_context_ = nullptr;
        lamia_enabled_ = false;
    }
}

void NotificationManager::process_notification(const Notification& notification) {
    notification_count_.fetch_add(1);
    
    // Send to registered handlers
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        for (auto& handler : handlers_) {
            try {
                handler->handle_notification(notification);
            } catch (const std::exception& e) {
                std::cerr << "❌ Notification handler error: " << e.what() << std::endl;
            }
        }
        
        // Send to callbacks
        for (auto& callback : callbacks_) {
            try {
                callback(notification);
            } catch (const std::exception& e) {
                std::cerr << "❌ Notification callback error: " << e.what() << std::endl;
            }
        }
    }
    
    // Send to Lamia backend if enabled
    if (lamia_enabled_ && lamia_context_) {
        send_to_lamia_backend(notification);
    }
    
    // Send to notification server if configured
    {
        std::lock_guard<std::mutex> lock(endpoint_mutex_);
        if (!notification_server_host_.empty() && notification_server_port_ > 0) {
            send_to_notification_server(notification);
        }
    }
}

void NotificationManager::send_to_lamia_backend(const Notification& notification) {
    if (!lamia_context_) {
        return;
    }
    
    // Convert category and severity to integers
    uint16_t category_val = static_cast<uint16_t>(notification.category);
    uint8_t severity_val = static_cast<uint8_t>(notification.severity);
    
    // Send to Lamia backend via C interface
    int result = notification_send(
        lamia_context_,
        notification.source_id.c_str(),
        notification.source_component.c_str(),
        category_val,
        severity_val,
        notification.title.c_str(),
        notification.message.c_str()
    );
    
    if (result != 0) {
        std::cerr << "❌ Failed to send notification to Lamia backend: " << result << std::endl;
    }
}

void NotificationManager::send_to_notification_server(const Notification& notification [[maybe_unused]]) {
    // TODO: Implement actual network send to MedusaServ Notification Server
    // This will use the async connection manager to send the notification
    // For now, just log that it would be sent
    std::cout << "📤 Would send notification to " << notification_server_host_ 
              << ":" << notification_server_port_ << std::endl;
}

} // namespace notifications
} // namespace network
} // namespace dualstack

// ============================================================================
// C Interface Implementation for Lamia Backend
// ============================================================================
// Must be in global namespace for C interface

// Define the NotificationContext struct (forward declared in header)
struct NotificationContext {
    dualstack::network::notifications::NotificationManager* manager;
    std::atomic<bool> active;
    
    NotificationContext() : manager(nullptr), active(true) {}
};

extern "C" {

NotificationContextHandle notification_context_create() {
    try {
        NotificationContext* ctx = new NotificationContext();
        ctx->manager = new dualstack::network::notifications::NotificationManager();
        if (!ctx->manager->initialize()) {
            delete ctx->manager;
            delete ctx;
            return nullptr;
        }
        return ctx;
    } catch (...) {
        return nullptr;
    }
}

void notification_context_destroy(NotificationContextHandle ctx) {
    if (!ctx) {
        return;
    }
    
    ctx->active = false;
    if (ctx->manager) {
        ctx->manager->shutdown();
        delete ctx->manager;
    }
    delete ctx;
}

int notification_send(NotificationContextHandle ctx,
                     const char* source_id,
                     const char* source_component,
                     uint16_t category,
                     uint8_t severity,
                     const char* title,
                     const char* message) {
    if (!ctx || !ctx->active || !ctx->manager) {
        return -1;
    }
    
    try {
        dualstack::network::notifications::Notification notification;
        notification.source_id = source_id ? source_id : "unknown";
        notification.source_component = source_component ? source_component : "unknown";
        notification.category = static_cast<dualstack::network::notifications::Category>(category);
        notification.severity = static_cast<dualstack::network::notifications::Severity>(severity);
        notification.title = title ? title : "";
        notification.message = message ? message : "";
        notification.timestamp = std::chrono::system_clock::now();
        
        ctx->manager->send_notification(notification);
        return 0;
    } catch (...) {
        return -1;
    }
}

} // extern "C"