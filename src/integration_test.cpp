#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <functional>

// 内存池
class MemPool {
public:
    MemPool(size_t size) : memory_(new char[size]), size_(size), offset_(0) {
        std::cout << "MemPool created: " << size << " bytes at " << (void*)memory_ << std::endl;
    }
    
    ~MemPool() {
        std::cout << "MemPool destroyed" << std::endl;
        delete[] memory_;
    }
    
    void* alloc(size_t n) {
        void* p = memory_ + offset_;
        offset_ += n;
        return p;
    }
    
private:
    char* memory_;
    size_t size_;
    size_t offset_;
};

// Reactor
class Reactor {
public:
    Reactor(int id) : id_(id), running_(false), epoll_fd_(-1) {
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ < 0) {
            throw std::runtime_error("epoll_create failed");
        }
    }
    
    ~Reactor() {
        stop();
        if (epoll_fd_ >= 0) close(epoll_fd_);
    }
    
    void start() {
        running_ = true;
        thread_ = std::thread(&Reactor::loop, this);
    }
    
    void stop() {
        running_ = false;
        if (epoll_fd_ >= 0) {
            char dummy = 0;
            ::write(epoll_fd_, &dummy, 1);
        }
        if (thread_.joinable()) thread_.join();
    }
    
    int epoll_fd() const { return epoll_fd_; }
    
private:
    void loop() {
        struct epoll_event ev[64];
        while (running_) {
            int n = epoll_wait(epoll_fd_, ev, 64, 100);
            if (n > 0) {
                for (int i = 0; i < n; i++) {
                    // 处理事件
                }
            }
        }
    }
    
    int id_;
    std::atomic<bool> running_;
    std::thread thread_;
    int epoll_fd_;
};

class ReactorPool {
public:
    ReactorPool(int count) {
        for (int i = 0; i < count; i++) {
            reactors_.push_back(std::make_unique<Reactor>(i));
        }
    }
    
    ~ReactorPool() {
        stop();
    }
    
    void start() {
        for (auto& r : reactors_) r->start();
    }
    
    void stop() {
        for (auto& r : reactors_) r->stop();
    }
    
    Reactor* next() {
        static std::atomic<int> idx{0};
        return reactors_[idx++ % reactors_.size()].get();
    }
    
private:
    std::vector<std::unique_ptr<Reactor>> reactors_;
};

// Acceptor
class Acceptor {
public:
    Acceptor(Reactor* reactor) : reactor_(reactor), fd_(-1), listening_(false) {
        fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    }
    
    ~Acceptor() {
        stop();
        if (fd_ >= 0) close(fd_);
    }
    
    bool listen(const std::string& ip, uint16_t port) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        
        int opt = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        if (::bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Bind failed: " << strerror(errno) << std::endl;
            return false;
        }
        
        if (::listen(fd_, 1024) < 0) {
            std::cerr << "Listen failed: " << strerror(errno) << std::endl;
            return false;
        }
        
        listening_ = true;
        std::cout << "Listening on port " << port << std::endl;
        return true;
    }
    
    void stop() { listening_ = false; }
    
    int fd() const { return fd_; }
    
private:
    Reactor* reactor_;
    int fd_;
    std::atomic<bool> listening_;
};

// 模拟的 RpcServer
class RpcServer {
public:
    RpcServer(int port) : port_(port), running_(false) {
        std::cout << "Creating RpcServer on port " << port << std::endl;
        
        pool_ = std::make_unique<ReactorPool>(2);
        acceptor_ = std::make_unique<Acceptor>(pool_->next());
        
        std::cout << "RpcServer created successfully" << std::endl;
    }
    
    ~RpcServer() {
        std::cout << "Destroying RpcServer..." << std::endl;
        stop();
    }
    
    bool start() {
        if (running_) return true;
        
        std::cout << "Starting RpcServer..." << std::endl;
        pool_->start();
        
        if (!acceptor_->listen("0.0.0.0", port_)) {
            std::cerr << "Failed to listen" << std::endl;
            return false;
        }
        
        running_ = true;
        std::cout << "RpcServer started successfully" << std::endl;
        return true;
    }
    
    void stop() {
        if (!running_) return;
        running_ = false;
        acceptor_->stop();
        pool_->stop();
        std::cout << "RpcServer stopped" << std::endl;
    }
    
private:
    int port_;
    std::atomic<bool> running_;
    std::unique_ptr<ReactorPool> pool_;
    std::unique_ptr<Acceptor> acceptor_;
};

int main() {
    std::cout << "=== Full Integration Test ===" << std::endl;
    
    std::cout << "\nStep 1: Creating server..." << std::endl;
    RpcServer server(9876);
    
    std::cout << "\nStep 2: Starting server..." << std::endl;
    if (!server.start()) {
        std::cerr << "Start failed!" << std::endl;
        return 1;
    }
    
    std::cout << "\nStep 3: Server running for 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    std::cout << "\nStep 4: Stopping server..." << std::endl;
    server.stop();
    
    std::cout << "\n=== Integration Test Completed Successfully! ===" << std::endl;
    
    return 0;
}
