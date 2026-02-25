#ifndef RPC_TYPES_H
#define RPC_TYPES_H

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>

namespace rpc {

// 协议版本
constexpr uint8_t PROTOCOL_VERSION = 0x01;

// 消息类型
enum class MessageType : uint8_t {
    REQUEST = 0x01,
    RESPONSE = 0x02,
    HEARTBEAT = 0x03,
    ERROR = 0x04
};

// RPC 请求结构
struct RpcRequest {
    uint64_t request_id;          // 请求 ID
    std::string service_name;     // 服务名
    std::string method_name;      // 方法名
    std::string params;           // 参数 (JSON 格式)
    std::string caller_id;        // 调用者 ID
};

// RPC 响应结构
struct RpcResponse {
    uint64_t request_id;          // 请求 ID
    bool success;                 // 是否成功
    std::string result;           // 结果 (JSON 格式)
    std::string error;            // 错误信息
};

// 服务注册信息
struct ServiceInfo {
    std::string service_name;
    std::string address;
    uint16_t port;
    uint64_t weight;              // 权重，用于负载均衡
    uint64_t last_heartbeat;      // 最后心跳时间
    bool available;               // 是否可用
};

// 连接配置
struct ConnectionConfig {
    int backlog = 1024;
    int timeout_ms = 30000;
    uint32_t buffer_size = 65536;
    bool enable_tcp_nodelay = true;
};

// 服务器配置
struct ServerConfig {
    std::string bind_ip;
    uint16_t port;
    int io_threads;               // Reactor 线程数
    int max_connections = 100000;
    uint32_t memory_pool_size = 1024 * 1024 * 100; // 100MB
    int heartbeat_interval_ms = 5000;
    int heartbeat_timeout_ms = 15000;
};

// 服务处理函数类型
using RpcHandler = std::function<std::string(const RpcRequest&)>;

// 服务注册表
class ServiceRegistry {
public:
    void register_service(const std::string& name, RpcHandler handler);
    RpcHandler get_handler(const std::string& name) const;
    size_t service_count() const;
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RpcHandler> handlers_;
};

} // namespace rpc

#endif // RPC_TYPES_H
