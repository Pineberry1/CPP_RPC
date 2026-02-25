#include "rpc/rpc_server.h"
#include "rpc/connection.h"
#include <iostream>
#include <csignal>
#include <unistd.h>

namespace rpc {

RpcServer::RpcServer(const ServerConfig& config)
    : config_(config),
      acceptor_reactor_idx_(0),
      memory_pool_(std::make_unique<MemoryPool>(config.memory_pool_size)),
      reactor_pool_(std::make_unique<ReactorPool>(config.io_threads, memory_pool_.get())),
      running_(false),
      heartbeat_thread_stop_(false) {
    
    discovery_ = std::make_unique<MemoryServiceDiscovery>();
    hash_ring_ = std::make_unique<ConsistentHashRing>();
    
    std::cout << "RpcServer constructed (not started yet)" << std::endl;
}

RpcServer::~RpcServer() {
    stop();
}

bool RpcServer::start() {
    if (running_) {
        return true;
    }
    
    std::cout << "Starting RpcServer..." << std::endl;
    
    // 启动 Reactor 线程池
    reactor_pool_->start();
    
    // 在 Reactor 线程池启动后，创建 acceptor
    acceptor_ = std::make_unique<Acceptor>(
        reactor_pool_->get_reactor(acceptor_reactor_idx_),
        ConnectionConfig{}
    );
    
    acceptor_->set_connection_callback([this](int fd, const struct sockaddr_in& addr) {
        handle_connection(fd, addr);
    });
    
    // 启动监听
    if (!acceptor_->listen(config_.bind_ip, config_.port)) {
        std::cerr << "Failed to listen on " << config_.bind_ip << ":" << config_.port << std::endl;
        return false;
    }
    
    running_ = true;
    
    // 启动心跳线程
    heartbeat_thread_ = std::thread(&RpcServer::heartbeat_thread_func, this);
    
    std::cout << "RPC Server started on " << config_.bind_ip << ":" << config_.port << std::endl;
    
    return true;
}

void RpcServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    heartbeat_thread_stop_ = true;
    
    // 停止心跳线程
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    
    // 关闭所有连接
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& pair : connections_) {
            pair.second->close();
        }
    }
    
    acceptor_->stop();
    reactor_pool_->stop();
    
    std::cout << "RPC Server stopped" << std::endl;
}

void RpcServer::register_service(const std::string& name, RpcHandler handler) {
    registry_.register_service(name, handler);
    std::cout << "Registered service: " << name << std::endl;
}

RpcHandler RpcServer::get_handler(const std::string& name) const {
    return registry_.get_handler(name);
}

void RpcServer::handle_connection(int fd, const struct sockaddr_in& addr) {
    // 创建连接
    auto connection = std::make_shared<Connection>(
        fd,
        addr,
        reactor_pool_->get_reactor(acceptor_reactor_idx_ + 1),  // 使用另一个 reactor 处理连接
        memory_pool_.get()
    );
    
    int connection_fd = fd;
    // 设置回调
    connection->set_message_callback([this, connection_fd](const void* data, size_t len) {
        handle_message(connection_fd, static_cast<const uint8_t*>(data), len);
    });
    
    connection->set_close_callback([this, fd]() {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(fd);
    });
    
    // 添加到连接管理
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[fd] = connection;
    }
    
    std::cout << "New connection from " << connection->address() 
              << ", total connections: " << connections_.size() << std::endl;
}

void RpcServer::handle_message(int fd, const uint8_t* data, size_t len) {
    // 解析消息
    TlvHeader header;
    std::memcpy(&header, data, sizeof(header));
    
    switch (static_cast<MessageType>(header.type)) {
        case MessageType::REQUEST: {
            RpcRequest request;
            if (TlvProtocol::decode_request(data, len, request)) {
                process_request(request, fd);
            } else {
                send_error(fd, header.request_id, "Invalid request format");
            }
            break;
        }
        
        case MessageType::HEARTBEAT: {
            if (TlvProtocol::decode_heartbeat(data, len)) {
                // 记录心跳
                std::string addr = "";
                auto it = connections_.find(fd);
                if (it != connections_.end()) {
                    addr = it->second->address();
                }
                // TODO: 更新服务发现
            }
            break;
        }
        
        default:
            send_error(fd, header.request_id, "Unknown message type");
    }
}

void RpcServer::process_request(const RpcRequest& request, int fd) {
    // 查找处理函数
    RpcHandler handler = registry_.get_handler(request.service_name);
    
    if (!handler) {
        send_error(fd, request.request_id, "Service not found: " + request.service_name);
        return;
    }
    
    try {
        // 调用服务
        std::string result = handler(request);
        
        // 返回响应
        RpcResponse response;
        response.request_id = request.request_id;
        response.success = true;
        response.result = result;
        
        auto tlv = TlvProtocol::encode_response(response, request.request_id);
        send_response(fd, response, request.request_id);
        
    } catch (const std::exception& e) {
        send_error(fd, request.request_id, std::string("Exception: ") + e.what());
    }
}

void RpcServer::send_response(int fd, const RpcResponse& response, uint64_t request_id) {
    auto tlv = TlvProtocol::encode_response(response, request_id);
    
    auto it = connections_.find(fd);
    if (it != connections_.end()) {
        it->second->send_tlv(tlv.data(), tlv.size());
    }
}

void RpcServer::send_error(int fd, uint64_t request_id, const std::string& error) {
    RpcResponse response;
    response.request_id = request_id;
    response.success = false;
    response.error = error;
    
    auto tlv = TlvProtocol::encode_response(response, request_id);
    
    auto it = connections_.find(fd);
    if (it != connections_.end()) {
        it->second->send_tlv(tlv.data(), tlv.size());
    }
}

void RpcServer::heartbeat_thread_func() {
    while (!heartbeat_thread_stop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.heartbeat_interval_ms));
        check_connections();
    }
}

void RpcServer::check_connections() {
    auto now = std::chrono::system_clock::now();
    auto timeout = std::chrono::seconds(config_.heartbeat_timeout_ms / 1000);
    
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    for (auto it = connections_.begin(); it != connections_.end(); ) {
        auto conn = it->second;
        // TODO: 检查连接活跃状态
        ++it;
    }
}

} // namespace rpc
