#ifndef RPC_SERVICE_DISCOVERY_H
#define RPC_SERVICE_DISCOVERY_H

#include "rpc_types.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>

namespace rpc {

// 服务发现接口
class ServiceDiscovery {
public:
    virtual ~ServiceDiscovery() = default;
    
    // 注册服务
    virtual bool register_service(const ServiceInfo& info) = 0;
    
    // 注销服务
    virtual bool unregister_service(const std::string& service_name, 
                                     const std::string& address) = 0;
    
    // 获取服务列表
    virtual std::vector<ServiceInfo> get_services(const std::string& service_name) = 0;
    
    // 获取可用服务
    virtual std::vector<ServiceInfo> get_available_services(const std::string& service_name) = 0;
    
    // 心跳检测
    virtual void heartbeat(const std::string& service_name, 
                          const std::string& address,
                          uint16_t port) = 0;
    
    // 设置心跳回调
    virtual void set_heartbeat_interval(int interval_ms) = 0;
};

// 内存服务发现实现
class MemoryServiceDiscovery : public ServiceDiscovery {
public:
    MemoryServiceDiscovery();
    
    bool register_service(const ServiceInfo& info) override;
    bool unregister_service(const std::string& service_name, 
                            const std::string& address) override;
    std::vector<ServiceInfo> get_services(const std::string& service_name) override;
    std::vector<ServiceInfo> get_available_services(const std::string& service_name) override;
    void heartbeat(const std::string& service_name, 
                   const std::string& address,
                   uint16_t port) override;
    void set_heartbeat_interval(int interval_ms) override;
    
private:
    void cleanup_dead_services();
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<ServiceInfo>> services_;
    int heartbeat_interval_ms_;
    std::chrono::steady_clock::time_point last_cleanup_;
};

} // namespace rpc

#endif // RPC_SERVICE_DISCOVERY_H
