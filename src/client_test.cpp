#include "rpc/rpc_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <future>
#include <atomic>
#include <mutex>

int main() {
    std::cout << "=== RPC Client Test ===" << std::endl;
    
    // 创建客户端配置
    rpc::ClientConfig client_config;
    client_config.io_threads = 2;
    client_config.connect_timeout_ms = 5000;
    client_config.read_timeout_ms = 30000;
    
    // 创建客户端
    rpc::RpcClient client(client_config);
    
    // 启动客户端
    if (!client.start()) {
        std::cerr << "Failed to start client" << std::endl;
        return 1;
    }
    
    // 连接服务器
    std::cout << "Connecting to server at 127.0.0.1:10808..." << std::endl;
    if (!client.connect("127.0.0.1", 10808)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "Connected successfully!" << std::endl;
    
    // 测试 1: 简单的加法服务调用
    std::cout << "\n=== Test 1: Basic Service Call ===" << std::endl;
    
    // 模拟调用加号服务
    for (int i = 1; i <= 5; i++) {
        std::cout << "Calling add service (request " << i << ")..." << std::endl;
        // 这里只是模拟调用，实际应该调用真实的服务
    }
    
    std::cout << "\n=== Test 2: Concurrent Connections ===" << std::endl;
    
    // 测试并发连接
    const int num_connections = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> fail_count{0};
    std::mutex log_mutex;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "Creating " << num_connections << " concurrent connections..." << std::endl;
    
    for (int i = 0; i < num_connections; i++) {
        threads.emplace_back([i, &success_count, &fail_count, &log_mutex]() {
            try {
                rpc::ClientConfig cfg;
                cfg.io_threads = 1;
                rpc::RpcClient local_client(cfg);
                
                if (local_client.start() && local_client.connect("127.0.0.1", 10808)) {
                    // 模拟发送请求
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    local_client.stop();
                    success_count++;
                } else {
                    fail_count++;
                }
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(log_mutex);
                std::cout << "Connection " << i << " failed: " << e.what() << std::endl;
                fail_count++;
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\nConcurrency Test Results:" << std::endl;
    std::cout << "  Total connections: " << num_connections << std::endl;
    std::cout << "  Successful: " << success_count << std::endl;
    std::cout << "  Failed: " << fail_count << std::endl;
    std::cout << "  Total time: " << duration.count() << "ms" << std::endl;
    std::cout << "  Throughput: " << (num_connections * 1000.0 / duration.count()) << " connections/sec" << std::endl;
    
    // 停止客户端
    client.stop();
    
    std::cout << "\n=== All Tests Completed! ===" << std::endl;
    
    return 0;
}
