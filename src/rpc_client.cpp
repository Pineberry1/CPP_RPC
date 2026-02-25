#include "rpc/rpc_client.h"
#include "rpc/connection.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

namespace rpc {

RpcClient::RpcClient(const ClientConfig& config)
    : next_request_id_(1),
      memory_pool_(1024 * 1024 * 50), // 50MB
      reactor_pool_(std::make_unique<ReactorPool>(config.io_threads, &memory_pool_)),
      running_(false) {
    
    hash_ring_ = std::make_unique<ConsistentHashRing>();
}

RpcClient::~RpcClient() {
    stop();
}

bool RpcClient::start() {
    if (running_) {
        return true;
    }
    
    reactor_pool_->start();
    running_ = true;
    
    // 启动事件循环
    event_loop_thread_ = std::thread(&RpcClient::event_loop, this);
    
    std::cout << "RPC Client started" << std::endl;
    
    return true;
}

void RpcClient::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (event_loop_thread_.joinable()) {
        event_loop_thread_.join();
    }
    
    reactor_pool_->stop();
    
    // 关闭所有连接
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& pair : connections_) {
            pair.second->close();
        }
    }
    
    std::cout << "RPC Client stopped" << std::endl;
}

bool RpcClient::connect(const std::string& address, uint16_t port) {
    // 创建 Socket
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // 设置地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr);
    
    // 发起连接
    int ret = ::connect(fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr));
    if (ret < 0 && errno != EINPROGRESS) {
        close(fd);
        return false;
    }
    
    // 创建连接对象
    struct sockaddr_in dummy_addr;
    memset(&dummy_addr, 0, sizeof(dummy_addr));
    dummy_addr.sin_family = AF_INET;
    dummy_addr.sin_port = htons(port);
    
    auto connection = std::make_shared<Connection>(
        fd,
        dummy_addr,
        reactor_pool_->next_reactor(),
        &memory_pool_
    );
    
    // 设置回调
    connection->set_message_callback([this](const void* data, size_t len) {
        TlvHeader header;
        std::memcpy(&header, data, sizeof(header));
        
        if (header.type == static_cast<uint8_t>(MessageType::RESPONSE)) {
            RpcResponse response;
            if (TlvProtocol::decode_response(static_cast<const uint8_t*>(data), len, response)) {
                handle_response(response, response.request_id);
            }
        }
    });
    
    connection->set_close_callback([this, address]() {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(address);
    });
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[address] = connection;
    }
    
    std::cout << "Connected to " << address << ":" << port << std::endl;
    
    return true;
}

void RpcClient::disconnect(const std::string& address) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(address);
    if (it != connections_.end()) {
        it->second->close();
        connections_.erase(it);
    }
}

RpcResponse RpcClient::call(const std::string& service_name,
                             const std::string& method_name,
                             const std::string& params,
                             int timeout_ms) {
    auto future = call_async(service_name, method_name, params, timeout_ms);
    return future.get();
}

std::future<RpcResponse> RpcClient::call_async(const std::string& service_name,
                                                 const std::string& method_name,
                                                 const std::string& params,
                                                 int timeout_ms) {
    std::future<RpcResponse> future;
    
    // 选择节点
    std::string node_id = hash_ring_->get_node(service_name);
    if (node_id.empty()) {
        std::cerr << "No available node for service: " << service_name << std::endl;
        return future;
    }
    
    // TODO: 解析节点地址
    
    // 发送请求
    uint64_t request_id = next_request_id_++;
    
    RpcRequest request;
    request.service_name = service_name;
    request.method_name = method_name;
    request.params = params;
    request.caller_id = "client";
    
    auto tlv = TlvProtocol::encode_request(request, request_id);
    
    // TODO: 发送到对应节点
    
    return future;
}

void RpcClient::send_heartbeat(const std::string& service_name) {
    // TODO: 发送心跳
}

void RpcClient::event_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cleanup_expired_requests();
    }
}

void RpcClient::handle_response(const RpcResponse& response, uint64_t request_id) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    
    auto it = pending_requests_.find(request_id);
    if (it != pending_requests_.end()) {
        it->second.promise.set_value(response);
        pending_requests_.erase(it);
    }
}

void RpcClient::cleanup_expired_requests() {
    auto now = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(pending_mutex_);
    
    for (auto it = pending_requests_.begin(); it != pending_requests_.end(); ) {
        if (now > it->second.timeout) {
            it->second.promise.set_value(RpcResponse{});
            it = pending_requests_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace rpc
