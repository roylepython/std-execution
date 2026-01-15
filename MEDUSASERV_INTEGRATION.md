# MedusaServ Integration Guide
## Amphisbaena (std_execution) Library

### Overview
Amphisbaena provides dual-stack (IPv4/IPv6) networking with std::execution for MedusaServ integration.

### Port Configuration
- **Amphisbaena ADS-RDR**: Port 42 (the answer to life, the universe, and everything)
- **Key Service**: Port 84
- **HTTPS**: Port 443 (standard)

### Integration Points for MedusaServ

#### 1. Dual-Stack Server
```cpp
#include "dualstack_net26/network/async_connection_manager.h"

using namespace dualstack::network;

// Create dual-stack server for MedusaServ
AsyncDualStackServer server(443);  // HTTPS port
server.set_connection_handler([](std::string conn_id, Socket sock, const IPAddress& addr) {
    // Handle MedusaServ connections
});
server.start();
```

#### 2. Notification System
```cpp
#include "dualstack_net26/network/notifications.h"

using namespace dualstack::network::notifications;

NotificationManager notif_mgr;
notif_mgr.initialize();
notif_mgr.set_notification_server_endpoint("medusaserv.local", 443);
notif_mgr.send_session_event("session_123", "CONNECTED", "User connected");
```

#### 3. MedusaServ VHost Integration
- Use `AsyncDualStackServer` for virtual host management
- Port 42 for ADS-RDR (Address Resolution - Data Routing)
- Port 84 for key management service
- Full TLS/SSL support via LFSSL integration

### Environment Independence
The library works regardless of environment:
- **Installed Package**: `find_package(DualStackNet26 REQUIRED)`
- **Source Build**: `add_subdirectory(../std_execution)`
- **Cross-platform**: Windows, Linux, WSL, Ubuntu

### MedusaServ Features Supported
- ✅ Dual-stack networking (IPv4/IPv6)
- ✅ Async connection management
- ✅ Notification system
- ✅ GalaxyCDN protocol support
- ✅ TLS/SSL ready (via LFSSL)
- ✅ Session management
- ✅ User event tracking

