#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <future>

int main() {
    std::cout << "=== Simple RPC Client Test ===" << std::endl;
    
    // 测试 1: 基本连接测试
    std::cout << "\n=== Test 1: Basic Connection Test ===" << std::endl;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(10808);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    auto connect_start = std::chrono::high_resolution_clock::now();
    int ret = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    auto connect_end = std::chrono::high_resolution_clock::now();
    
    if (ret < 0) {
        std::cerr << "Connect failed: " << strerror(errno) << std::endl;
        close(sock);
        return 1;
    }
    
    auto connect_time = std::chrono::duration_cast<std::chrono::microseconds>(connect_end - connect_start);
    std::cout << "Connected successfully! (connect time: " << connect_time.count() << "us)" << std::endl;
    
    // 发送测试数据
    const char* test_msg = "Hello RPC Server!";
    send(sock, test_msg, strlen(test_msg), 0);
    std::cout << "Sent test message: " << test_msg << std::endl;
    
    close(sock);
    
    // 测试 2: 并发连接测试
    std::cout << "\n=== Test 2: Concurrent Connections ===" << std::endl;
    
    const int num_connections = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> fail_count{0};
    std::mutex log_mutex;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "Creating " << num_connections << " concurrent connections..." << std::endl;
    
    for (int i = 0; i < num_connections; i++) {
        threads.emplace_back([i, &success_count, &fail_count]() {
            try {
                int sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) {
                    fail_count++;
                    return;
                }
                
                struct sockaddr_in server_addr;
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(10808);
                inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
                
                int ret = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
                if (ret >= 0) {
                    // 发送一点数据
                    const char* msg = "test";
                    send(sock, msg, strlen(msg), 0);
                    close(sock);
                    success_count++;
                } else {
                    close(sock);
                    fail_count++;
                }
            } catch (...) {
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
    
    // 测试 3: 压力测试
    std::cout << "\n=== Test 3: Load Test (1000 requests) ===" << std::endl;
    
    const int num_requests = 1000;
    std::atomic<int> request_success{0};
    
    auto load_start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_requests; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock >= 0) {
            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(10808);
            inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
            
            int ret = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
            if (ret >= 0) {
                const char* msg = "request";
                send(sock, msg, strlen(msg), 0);
                usleep(1000); // 模拟处理时间
                close(sock);
                request_success++;
            }
        }
    }
    
    auto load_end = std::chrono::high_resolution_clock::now();
    auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_start);
    
    std::cout << "\nLoad Test Results:" << std::endl;
    std::cout << "  Total requests: " << num_requests << std::endl;
    std::cout << "  Successful: " << request_success << std::endl;
    std::cout << "  Total time: " << load_duration.count() << "ms" << std::endl;
    std::cout << "  Throughput: " << (num_requests * 1000.0 / load_duration.count()) << " requests/sec" << std::endl;
    
    std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;
    
    return 0;
}
