#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include "rpc_types.h"
#include "reactor.h"
#include "memory_pool.h"
#include "tlv_protocol.h"
#include "hash_ring.h"
#include "connection.h"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>

namespace rpc {

// RPC 客户端配置
struct ClientConfig {
    int io_threads = 2;
    int connect_timeout_ms = 5000;
    int read_timeout_ms = 30000;
    int write_timeout_ms = 30000;
    uint32_t buffer_size = 65536;
};

// 请求等待器
struct PendingRequest {
    uint64_t request_id;
    std::promise<RpcResponse> promise;
    std::chrono::steady_clock::time_point timeout;
};

// RPC 客户端
class RpcClient {
public:
    explicit RpcClient(const ClientConfig& config);
    ~RpcClient();

    // 启动客户端
    bool start();
    
    // 停止客户端
    void stop();
    
    // 连接到服务
    bool connect(const std::string& address, uint16_t port);
    
    // 断开连接
    void disconnect(const std::string& address);
    
    // 调用服务 (同步)
    RpcResponse call(const std::string& service_name, 
                     const std::string& method_name,
                     const std::string& params,
                     int timeout_ms = 30000);
    
    // 调用服务 (异步)
    std::future<RpcResponse> call_async(const std::string& service_name,
                                         const std::string& method_name,
                                         const std::string& params,
                                         int timeout_ms = 30000);
    
    // 发送心跳
    void send_heartbeat(const std::string& service_name);
    
    // 是否运行中
    bool is_running() const { return running_; }
    
    // 获取内存池
    MemoryPool* memory_pool() { return &memory_pool_; }

private:
    void event_loop();
    void handle_response(const RpcResponse& response, uint64_t request_id);
    void cleanup_expired_requests();
    
    ClientConfig config_;
    MemoryPool memory_pool_;
    
    std::unique_ptr<ReactorPool> reactor_pool_;
    
    std::unordered_map<std::string, ConnectionPtr> connections_;
    std::mutex connections_mutex_;
    
    std::unordered_map<uint64_t, PendingRequest> pending_requests_;
    std::mutex pending_mutex_;
    uint64_t next_request_id_;
    
    std::atomic<bool> running_;
    std::thread event_loop_thread_;
    
    // 服务到节点的映射
    std::unique_ptr<ConsistentHashRing> hash_ring_;
    std::unordered_map<std::string, std::string> service_to_node_;
    std::mutex service_mutex_;
};

} // namespace rpc

#endif // RPC_CLIENT_H
