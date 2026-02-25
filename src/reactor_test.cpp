#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <unistd.h>
#include <sys/epoll.h>
#include <cstring>

// 简单的 Reactor 测试（包含 epoll）
class SimpleReactor {
public:
    SimpleReactor(int id) : id_(id), running_(false), epoll_fd_(-1) {
        std::cout << "Creating reactor " << id_ << " (epoll_create)..." << std::endl;
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ < 0) {
            std::cerr << "Failed to create epoll: " << strerror(errno) << std::endl;
            throw std::runtime_error("epoll_create failed");
        }
        std::cout << "Reactor " << id_ << " epoll_fd=" << epoll_fd_ << std::endl;
    }
    
    ~SimpleReactor() {
        std::cout << "Destroying reactor " << id_ << std::endl;
        stop();
        if (epoll_fd_ >= 0) {
            close(epoll_fd_);
        }
    }
    
    void start() {
        std::cout << "Starting reactor " << id_ << std::endl;
        running_ = true;
        thread_ = std::thread(&SimpleReactor::loop, this);
    }
    
    void stop() {
        if (!running_) return;
        
        std::cout << "Stopping reactor " << id_ << std::endl;
        running_ = false;
        
        // 唤醒 epoll_wait
        if (epoll_fd_ >= 0) {
            char dummy = 0;
            ::write(epoll_fd_, &dummy, 1);
        }
        
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    
private:
    void loop() {
        struct epoll_event events[64];
        std::cout << "Reactor " << id_ << " event loop started" << std::endl;
        
        while (running_) {
            int n = epoll_wait(epoll_fd_, events, 64, 100); // 100ms timeout
            
            if (n < 0) {
                if (errno != EINTR) {
                    std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
                }
                continue;
            }
            
            std::cout << "Reactor " << id_ << " handled " << n << " events" << std::endl;
        }
        
        std::cout << "Reactor " << id_ << " event loop stopped" << std::endl;
    }
    
    int id_;
    std::atomic<bool> running_;
    std::thread thread_;
    int epoll_fd_;
};

class SimpleReactorPool {
public:
    SimpleReactorPool(int count) {
        std::cout << "Creating reactor pool with " << count << " reactors..." << std::endl;
        for (int i = 0; i < count; i++) {
            reactors_.push_back(std::make_unique<SimpleReactor>(i));
        }
    }
    
    ~SimpleReactorPool() {
        stop();
    }
    
    void start() {
        std::cout << "Starting reactor pool..." << std::endl;
        for (auto& r : reactors_) {
            r->start();
        }
    }
    
    void stop() {
        std::cout << "Stopping reactor pool..." << std::endl;
        for (auto& r : reactors_) {
            r->stop();
        }
    }
    
private:
    std::vector<std::unique_ptr<SimpleReactor>> reactors_;
};

int main() {
    std::cout << "=== Reactor with epoll Test ===" << std::endl;
    
    std::cout << "\nCreating reactor pool..." << std::endl;
    SimpleReactorPool pool(2);
    
    std::cout << "\nStarting reactor pool..." << std::endl;
    pool.start();
    
    std::cout << "\nWaiting 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    std::cout << "\nStopping reactor pool..." << std::endl;
    pool.stop();
    
    std::cout << "\n=== Test Completed Successfully! ===" << std::endl;
    
    return 0;
}
