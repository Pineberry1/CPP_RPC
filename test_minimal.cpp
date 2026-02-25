#include <iostream>
#include <thread>
#include <csignal>

// 最小化测试
int main() {
    std::cout << "=== Simple RPC Server Test ===" << std::endl;
    std::cout << "Creating thread pool..." << std::endl;
    
    // 简单的线程池测试
    std::vector<std::thread> threads;
    for(int i = 0; i < 2; i++) {
        threads.emplace_back([](){
            std::cout << "Worker thread running" << std::endl;
        });
    }
    
    for(auto& t : threads) {
        t.join();
    }
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}
