#include "rpc/rpc_server.h"
#include <iostream>
#include <thread>

// 简单服务
std::string hello_service(const rpc::RpcRequest& request) {
    return "{\"msg\": \"Hello from RPC!\"}";
}

int main() {
    std::cout << "=== RPC Server Starting ===" << std::endl;
    
    rpc::ServerConfig config;
    config.bind_ip = "127.0.0.1";
    config.port = 9000;
    config.io_threads = 2;
    config.memory_pool_size = 50 * 1024 * 1024;
    
    std::cout << "Creating server..." << std::endl;
    rpc::RpcServer server(config);
    
    std::cout << "Registering service..." << std::endl;
    server.register_service("HelloService", hello_service);
    
    std::cout << "Starting server..." << std::endl;
    if (!server.start()) {
        std::cerr << "Failed to start" << std::endl;
        return 1;
    }
    
    std::cout << "Server is running!" << std::endl;
    
    // 保持运行
    std::this_thread::sleep_for(std::chrono::hours(1));
    
    server.stop();
    return 0;
}
