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

class SimpleAcceptor {
public:
    SimpleAcceptor() : fd_(-1), listening_(false) {
        std::cout << "Creating acceptor..." << std::endl;
        fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd_ < 0) {
            std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
            throw std::runtime_error("socket failed");
        }
        std::cout << "Acceptor socket fd=" << fd_ << std::endl;
    }
    
    ~SimpleAcceptor() {
        std::cout << "Destroying acceptor..." << std::endl;
        stop();
        if (fd_ >= 0) {
            close(fd_);
        }
    }
    
    bool listen(const std::string& ip, uint16_t port) {
        std::cout << "Listening on " << ip << ":" << port << std::endl;
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        
        if (ip == "0.0.0.0" || ip.empty()) {
            addr.sin_addr.s_addr = INADDR_ANY;
        } else {
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        }
        
        int opt = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        if (::bind(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "Failed to bind: " << strerror(errno) << std::endl;
            return false;
        }
        
        if (::listen(fd_, 1024) < 0) {
            std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
            return false;
        }
        
        listening_ = true;
        return true;
    }
    
    void stop() {
        listening_ = false;
    }
    
    int fd() const { return fd_; }
    bool is_listening() const { return listening_; }
    
private:
    int fd_;
    std::atomic<bool> listening_;
};

int main() {
    std::cout << "=== Acceptor Test ===" << std::endl;
    
    std::cout << "\nCreating acceptor..." << std::endl;
    SimpleAcceptor acceptor;
    
    std::cout << "\nStarting to listen on port 9999..." << std::endl;
    if (!acceptor.listen("127.0.0.1", 9999)) {
        std::cerr << "Listen failed!" << std::endl;
        return 1;
    }
    
    std::cout << "\nServer listening. Accepting connections..." << std::endl;
    
    // 接受几个连接
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    for (int i = 0; i < 3; i++) {
        int client_fd = accept(acceptor.fd(), 
                               reinterpret_cast<struct sockaddr*>(&client_addr), 
                               &client_len);
        if (client_fd >= 0) {
            std::cout << "Accepted connection " << i+1 << " from fd=" << client_fd << std::endl;
            close(client_fd);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "No pending connections" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                std::cerr << "Accept error: " << strerror(errno) << std::endl;
            }
        }
    }
    
    std::cout << "\nStopping server..." << std::endl;
    acceptor.stop();
    
    std::cout << "\n=== Test Completed Successfully! ===" << std::endl;
    
    return 0;
}
