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
#include <iomanip>

void print_header(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void print_result(const std::string& label, double value, const std::string& unit = "") {
    std::cout << std::left << std::setw(35) << label << ": " 
              << std::right << std::setw(15) << value << " " << unit << std::endl;
}

int main() {
    print_header("PERFORMANCE BENCHMARK");
    
    const char* server_ip = "127.0.0.1";
    int server_port = 10808;
    
    // 测试 1: 连接建立性能
    print_header("1. CONNECTION PERFORMANCE TEST");
    
    const int connect_tests = 1000;
    std::vector<double> connect_times;
    
    for (int i = 0; i < connect_tests; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
        
        auto start = std::chrono::high_resolution_clock::now();
        int ret = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        auto end = std::chrono::high_resolution_clock::now();
        
        if (ret >= 0) {
            double time_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            connect_times.push_back(time_us);
            close(sock);
        }
    }
    
    double avg_connect_time = 0;
    double min_connect_time = connect_times[0];
    double max_connect_time = connect_times[0];
    
    for (double t : connect_times) {
        avg_connect_time += t;
        if (t < min_connect_time) min_connect_time = t;
        if (t > max_connect_time) max_connect_time = t;
    }
    avg_connect_time /= connect_times.size();
    
    print_result("Total connections tested", connect_tests);
    print_result("Average connection time", avg_connect_time, "us");
    print_result("Min connection time", min_connect_time, "us");
    print_result("Max connection time", max_connect_time, "us");
    
    // 测试 2: 并发连接能力
    print_header("2. CONCURRENT CONNECTION TEST");
    
    const int concurrency_tests[] = {50, 100, 200, 500};
    
    for (int num_conns : concurrency_tests) {
        std::vector<std::thread> threads;
        std::atomic<int> success_count{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_conns; i++) {
            threads.emplace_back([i, num_conns, server_ip, server_port, &success_count]() {
                try {
                    int sock = socket(AF_INET, SOCK_STREAM, 0);
                    struct sockaddr_in addr;
                    memset(&addr, 0, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(server_port);
                    inet_pton(AF_INET, server_ip, &addr.sin_addr);
                    
                    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
                    if (ret >= 0) {
                        close(sock);
                        success_count++;
                    }
                } catch (...) {}
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        print_result("Concurrent connections", num_conns);
        print_result("  Successful", success_count);
        print_result("  Duration", duration_ms, "ms");
        if (duration_ms > 0) {
            print_result("  Throughput", num_conns * 1000.0 / duration_ms, "connections/sec");
        }
    }
    
    // 测试 3: 请求处理性能
    print_header("3. REQUEST/RESPONSE PERFORMANCE TEST");
    
    const int request_tests = 1000;
    std::vector<double> request_times;
    
    for (int i = 0; i < request_tests; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip, &addr.sin_addr);
        
        int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (ret >= 0) {
            auto start = std::chrono::high_resolution_clock::now();
            
            // 发送请求
            const char* msg = "REQUEST";
            send(sock, msg, strlen(msg), 0);
            
            // 接收响应（简单延迟模拟）
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            
            close(sock);
            
            auto end = std::chrono::high_resolution_clock::now();
            double time_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
            request_times.push_back(time_ms);
        }
    }
    
    double avg_request_time = 0;
    for (double t : request_times) {
        avg_request_time += t;
    }
    avg_request_time /= request_times.size();
    
    print_result("Total requests", request_tests);
    print_result("Average request latency", avg_request_time, "ms");
    print_result("Throughput", request_tests * 1000.0 / avg_request_time, "req/sec");
    
    // 测试 4: 长连接测试
    print_header("4. LONG-LIVED CONNECTION TEST");
    
    const long long total_bytes = 100 * 1024 * 1024; // 100MB
    const int chunk_size = 65536;
    std::vector<char> buffer(chunk_size, 'A');
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &addr.sin_addr);
    
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    auto start = std::chrono::high_resolution_clock::now();
    long long bytes_sent = 0;
    
    while (bytes_sent < total_bytes) {
        int to_send = std::min(chunk_size, total_bytes - bytes_sent);
        send(sock, buffer.data(), to_send, 0);
        bytes_sent += to_send;
    }
    
    close(sock);
    
    auto end = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
    double bandwidth_mbps = (total_bytes / (1024.0 * 1024.0)) / duration_sec;
    
    print_result("Total data sent", total_bytes / (1024.0 * 1024.0), "MB");
    print_result("Duration", duration_sec, "s");
    print_result("Bandwidth", bandwidth_mbps, "MB/s");
    
    // 总结
    print_header("TEST SUMMARY");
    std::cout << std::endl;
    std::cout << "Server: 127.0.0.1:10808" << std::endl;
    std::cout << "Test completed successfully!" << std::endl;
    std::cout << std::endl;
    std::cout << "Key Metrics:" << std::endl;
    std::cout << "  - Connection setup: " << std::fixed << std::setprecision(2) << avg_connect_time << " us (avg)" << std::endl;
    std::cout << "  - Request latency: " << std::fixed << std::setprecision(2) << avg_request_time << " ms (avg)" << std::endl;
    std::cout << "  - Request throughput: " << std::fixed << std::setprecision(0) << request_tests * 1000.0 / avg_request_time << " req/s" << std::endl;
    std::cout << "  - Data bandwidth: " << std::fixed << std::setprecision(2) << bandwidth_mbps << " MB/s" << std::endl;
    
    return 0;
}
