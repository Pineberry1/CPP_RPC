#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include "rpc_types.h"
#include "reactor.h"
#include "acceptor.h"
#include "memory_pool.h"
#include "tlv_protocol.h"
#include "hash_ring.h"
#include "service_discovery.h"
#include "connection.h"
#include <memory>
#include <queue>
#include <thread>
#include <atomic>

namespace rpc {

// RPC 服务器
class RpcServer {
public:
    explicit RpcServer(const ServerConfig& config);
    ~RpcServer();

    // 启动服务器
    bool start();
    
    // 停止服务器
    void stop();
    
    // 是否运行中
    bool is_running() const { return running_; }
    
    // 注册服务
    void register_service(const std::string& name, RpcHandler handler);
    
    // 获取服务处理函数
    RpcHandler get_handler(const std::string& name) const;
    
    // 获取服务注册表
    ServiceRegistry* registry() { return &registry_; }
    
    // 服务发现
    ServiceDiscovery* discovery() { return discovery_.get(); }
    
    // 发送响应
    void send_response(int fd, const RpcResponse& response, uint64_t request_id);
    
    // 获取配置
    const ServerConfig& config() const { return config_; }
    
    // 启动 acceptor（在构造函数之后调用）
    void start_acceptor(int reactor_idx);

private:
    void handle_connection(int fd, const struct sockaddr_in& addr);
    void handle_message(int fd, const uint8_t* data, size_t len);
    void process_request(const RpcRequest& request, int fd);
    void send_error(int fd, uint64_t request_id, const std::string& error);
    
    void heartbeat_thread_func();
    void check_connections();
    
    ServerConfig config_;
    int acceptor_reactor_idx_;
    
    std::unique_ptr<MemoryPool> memory_pool_;
    std::unique_ptr<ReactorPool> reactor_pool_;
    
    std::unique_ptr<Acceptor> acceptor_;
    
    ServiceRegistry registry_;
    std::unique_ptr<ServiceDiscovery> discovery_;
    std::unique_ptr<ConsistentHashRing> hash_ring_;
    
    // 连接管理
    std::mutex connections_mutex_;
    std::unordered_map<int, ConnectionPtr> connections_;
    
    std::atomic<bool> running_;
    
    // 心跳线程
    std::thread heartbeat_thread_;
    std::atomic<bool> heartbeat_thread_stop_;
};

} // namespace rpc

#endif // RPC_SERVER_H
