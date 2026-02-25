#include "rpc/rpc_server.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

// 示例服务：加法服务
std::string add_service(const rpc::RpcRequest& request) {
    return "{\"result\": 42}";
}

// 服务存活标记
volatile bool server_running = true;

void signal_handler(int sig) {
    std::cout << "\nReceived signal " << sig << ", shutting down..." << std::endl;
    server_running = false;
}

int main() {
    std::cout << "=== High Performance RPC Framework Demo ===" << std::endl;
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建服务器配置
    rpc::ServerConfig server_config;
    server_config.bind_ip = "0.0.0.0";
    server_config.port = 10808;
    server_config.io_threads = 4;
    server_config.memory_pool_size = 1024 * 1024 * 200; // 200MB
    server_config.heartbeat_interval_ms = 5000;
    
    // 创建服务器
    rpc::RpcServer server(server_config);
    
    // 注册服务
    server.register_service("AddService", add_service);
    
    // 启动服务器
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Server running on port 10808. Press Ctrl+C to stop." << std::endl;
    
    // 后台运行循环
    while (server_running) {
        sleep(1);
    }
    
    server.stop();
    
    std::cout << "Server stopped." << std::endl;
    return 0;
}
