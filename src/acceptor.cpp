#include "rpc/acceptor.h"
#include "rpc/channel.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cstring>
#include <functional>

namespace rpc {

Acceptor::Acceptor(Reactor* reactor, const ConnectionConfig& config)
    : reactor_(reactor),
      config_(config),
      fd_(-1),
      channel_(nullptr),
      listening_(false) {
    
    fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // 设置地址重用
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

Acceptor::~Acceptor() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool Acceptor::listen(const std::string& bind_ip, uint16_t port) {
    if (listening_) {
        return true;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (bind_ip.empty() || bind_ip == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, bind_ip.c_str(), &addr.sin_addr);
    }
    
    if (::bind(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Failed to bind: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (::listen(fd_, config_.backlog) < 0) {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        return false;
    }
    
    // 创建 channel
    channel_ = new Channel(fd_);
    channel_->set_read_callback([this](const void* data, size_t len) {
        handle_accept();
    });
    
    reactor_->add_channel(channel_);
    listening_ = true;
    
    std::cout << "Server listening on " << bind_ip << ":" << port << std::endl;
    
    return true;
}

void Acceptor::stop() {
    if (!listening_) {
        return;
    }
    
    listening_ = false;
    
    if (channel_) {
        reactor_->delete_channel(channel_);
        delete channel_;
        channel_ = nullptr;
    }
    
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

void Acceptor::handle_accept() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (true) {
        int client_fd = accept(fd_, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
        
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 没有更多连接
            }
            std::cerr << "Accept error: " << strerror(errno) << std::endl;
            break;
        }
        
        // 设置非阻塞
        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
        
        // 启用 TCP_NODELAY
        int opt = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
        
        // 调用连接回调
        if (connection_cb_) {
            connection_cb_(client_fd, client_addr);
        }
    }
}

} // namespace rpc
