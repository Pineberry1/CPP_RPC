#include "rpc/service_discovery.h"
#include <chrono>
#include <iostream>

namespace rpc {

MemoryServiceDiscovery::MemoryServiceDiscovery()
    : heartbeat_interval_ms_(5000),
      last_cleanup_(std::chrono::steady_clock::now()) {
}

bool MemoryServiceDiscovery::register_service(const ServiceInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& services = services_[info.service_name];
    
    // 检查是否已存在
    for (auto& s : services) {
        if (s.address == info.address && s.port == info.port) {
            s = info; // 更新
            return true;
        }
    }
    
    services.push_back(info);
    return true;
}

bool MemoryServiceDiscovery::unregister_service(const std::string& service_name, 
                                                 const std::string& address) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return false;
    }
    
    it->second.erase(
        std::remove_if(it->second.begin(), it->second.end(),
            [&address](const ServiceInfo& s) {
                return s.address == address;
            }),
        it->second.end()
    );
    
    return true;
}

std::vector<ServiceInfo> MemoryServiceDiscovery::get_services(const std::string& service_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return {};
    }
    
    return it->second;
}

std::vector<ServiceInfo> MemoryServiceDiscovery::get_available_services(const std::string& service_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return {};
    }
    
    std::vector<ServiceInfo> available;
    for (const auto& s : it->second) {
        if (s.available) {
            available.push_back(s);
        }
    }
    
    return available;
}

void MemoryServiceDiscovery::heartbeat(const std::string& service_name, 
                                        const std::string& address,
                                        uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(service_name);
    if (it == services_.end()) {
        return;
    }
    
    for (auto& s : it->second) {
        if (s.address == address && s.port == port) {
            s.available = true;
            s.last_heartbeat = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            break;
        }
    }
}

void MemoryServiceDiscovery::set_heartbeat_interval(int interval_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    heartbeat_interval_ms_ = interval_ms;
}

void MemoryServiceDiscovery::cleanup_dead_services() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (auto& pair : services_) {
        for (auto& service : pair.second) {
            if (now - service.last_heartbeat > heartbeat_interval_ms_ / 1000 * 3) {
                service.available = false;
            }
        }
    }
    
    last_cleanup_ = std::chrono::steady_clock::now();
}

} // namespace rpc
