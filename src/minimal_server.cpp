#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

class MinServer {
public:
    MinServer(int port) : port_(port), running_(false) {
        std::cout << "Creating MinServer on port " << port << std::endl;
        setup();
    }
    
    ~MinServer() {
        stop();
    }
    
    void setup() {
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ < 0) {
            throw std::runtime_error("epoll_create failed");
        }
        
        sock_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sock_ < 0) {
            throw std::runtime_error("socket failed");
        }
        
        int opt = 1;
        setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);
        
        if (::bind(sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            throw std::runtime_error("bind failed");
        }
        
        if (::listen(sock_, 1024) < 0) {
            throw std::runtime_error("listen failed");
        }
        
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sock_;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock_, &ev);
        
        std::cout << "Server setup complete" << std::endl;
    }
    
    bool start() {
        if (running_) return true;
        
        running_ = true;
        loop_thread_ = std::thread(&MinServer::loop, this);
        
        std::cout << "Server started on port " << port_ << std::endl;
        return true;
    }
    
    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        // 唤醒 epoll
        char dummy = 0;
        ::write(epoll_fd_, &dummy, 1);
        
        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }
        
        if (sock_ >= 0) close(sock_);
        if (epoll_fd_ >= 0) close(epoll_fd_);
        
        std::cout << "Server stopped" << std::endl;
    }
    
private:
    void loop() {
        struct epoll_event ev[10];
        
        while (running_) {
            int n = epoll_wait(epoll_fd_, ev, 10, 100);
            if (n < 0) {
                if (errno != EINTR) {
                    std::cerr << "epoll_wait error" << std::endl;
                }
                continue;
            }
            
            for (int i = 0; i < n; i++) {
                if (ev[i].data.fd == sock_) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(sock_, (struct sockaddr*)&client_addr, &client_len);
                    if (client_fd >= 0) {
                        std::cout << "Accepted connection from fd=" << client_fd << std::endl;
                        close(client_fd);
                    }
                }
            }
        }
    }
    
    int port_;
    std::atomic<bool> running_;
    int sock_ = -1;
    int epoll_fd_ = -1;
    std::thread loop_thread_;
};

int main() {
    std::cout << "=== Minimal Server Test ===" << std::endl;
    
    MinServer server(9876);
    
    if (!server.start()) {
        std::cerr << "Start failed" << std::endl;
        return 1;
    }
    
    std::cout << "Server running. Press Enter to stop..." << std::endl;
    
    // 模拟运行 5 秒
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    server.stop();
    
    std::cout << "=== Test Completed Successfully! ===" << std::endl;
    
    return 0;
}
