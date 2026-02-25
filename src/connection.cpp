#include "rpc/connection.h"
#include "rpc/channel.h"
#include "rpc/reactor.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cstring>

namespace rpc {

Connection::Connection(int fd, const struct sockaddr_in& addr, Reactor* reactor, MemoryPool* pool)
    : fd_(fd),
      addr_(addr),
      reactor_(reactor),
      pool_(pool),
      state_(ConnectionState::CONNECTING),
      bytes_sent_(0),
      bytes_received_(0) {
    
    set_nonblocking(fd);
    set_tcp_nodelay(fd);
    
    // 创建 channel
    channel_ = new Channel(fd);
    channel_->set_read_callback([this](const void* data, size_t len) {
        handle_read();
    });
    channel_->set_write_callback([this]() {
        handle_write();
    });
    channel_->set_close_callback([this]() {
        handle_close();
    });
    
    // 添加到 reactor
    reactor_->add_channel(channel_);
}

Connection::~Connection() {
    if (channel_) {
        reactor_->delete_channel(channel_);
        delete channel_;
    }
}

void Connection::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Connection::set_tcp_nodelay(int fd) {
    int opt = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

void Connection::send(const void* data, size_t len) {
    if (state_ != ConnectionState::CONNECTED) {
        return;
    }
    
    // 追加到写缓冲
    write_buf_.insert(write_buf_.end(), static_cast<const char*>(data), 
                      static_cast<const char*>(data) + len);
    
    // 设置写事件
    channel_->set_events(EPOLLIN | EPOLLOUT);
    reactor_->update_channel(channel_);
}

void Connection::send_string(const std::string& data) {
    send(data.c_str(), data.size());
}

void Connection::send_tlv(const uint8_t* tlv, size_t tlv_len) {
    send(tlv, tlv_len);
}

void Connection::close() {
    if (state_ == ConnectionState::DISCONNECTED) {
        return;
    }
    
    state_ = ConnectionState::DISCONNECTING;
    
    // 停止写事件
    channel_->set_events(EPOLLIN);
    reactor_->update_channel(channel_);
}

std::string Connection::local_address() const {
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    
    if (getsockname(fd_, reinterpret_cast<struct sockaddr*>(&local_addr), &addr_len) >= 0) {
        char buf[64];
        inet_ntop(AF_INET, &local_addr.sin_addr, buf, sizeof(buf));
        return std::string(buf) + ":" + std::to_string(ntohs(local_addr.sin_port));
    }
    
    return "unknown";
}

void Connection::handle_read() {
    if (state_ != ConnectionState::CONNECTED) {
        return;
    }
    
    // 读取数据
    char buffer[65536];
    ssize_t n = read(fd_, buffer, sizeof(buffer));
    
    if (n > 0) {
        bytes_received_ += n;
        
        // 添加到读缓冲
        read_buf_.insert(read_buf_.end(), buffer, buffer + n);
        
        // 处理粘包/半包
        const uint8_t* data = reinterpret_cast<const uint8_t*>(read_buf_.data());
        size_t remaining = read_buf_.size();
        
        while (remaining >= HEADER_SIZE) {
            size_t msg_len = HEADER_SIZE;
            
            // 读取消息长度
            const TlvHeader* header = reinterpret_cast<const TlvHeader*>(data);
            if (TlvProtocol::validate_header(data)) {
                msg_len += header->length;
            }
            
            if (remaining >= msg_len) {
                // 完整消息，调用回调
                if (message_cb_) {
                    message_cb_(data, msg_len);
                }
                
                // 移除已处理的数据
                data += msg_len;
                remaining -= msg_len;
            } else {
                break; // 数据不完整
            }
        }
        
        // 更新缓冲
        if (remaining > 0) {
            std::memmove(read_buf_.data(), data, remaining);
            read_buf_.resize(remaining);
        } else {
            read_buf_.clear();
        }
    } else if (n == 0) {
        // 对端关闭连接
        handle_close();
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            handle_error(errno);
        }
    }
}

void Connection::handle_write() {
    if (state_ != ConnectionState::CONNECTED || write_buf_.empty()) {
        channel_->set_events(EPOLLIN);
        reactor_->update_channel(channel_);
        return;
    }
    
    ssize_t n = write(fd_, write_buf_.data(), write_buf_.size());
    
    if (n > 0) {
        bytes_sent_ += n;
        write_buf_.erase(write_buf_.begin(), write_buf_.begin() + n);
        
        if (write_buf_.empty()) {
            channel_->set_events(EPOLLIN);
            reactor_->update_channel(channel_);
        }
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        handle_error(errno);
    }
}

void Connection::handle_close() {
    state_ = ConnectionState::DISCONNECTED;
    
    if (close_cb_) {
        close_cb_();
    }
    
    // 从 reactor 删除
    reactor_->delete_channel(channel_);
}

void Connection::handle_error(int error) {
    state_ = ConnectionState::DISCONNECTED;
    handle_close();
}

} // namespace rpc
