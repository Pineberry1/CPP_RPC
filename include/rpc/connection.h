#ifndef RPC_CONNECTION_H
#define RPC_CONNECTION_H

#include "channel.h"
#include "memory_pool.h"
#include "tlv_protocol.h"
#include "rpc_types.h"
#include "reactor.h"
#include <string>
#include <cstring>
#include <algorithm>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

namespace rpc {

// 连接状态
enum class ConnectionState {
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    DISCONNECTED
};

// 连接类 - 管理单个 TCP 连接
class Connection {
public:
    Connection(int fd, const struct sockaddr_in& addr, Reactor* reactor, MemoryPool* pool);
    ~Connection();

    // 发送数据 (Zero-copy 优化)
    void send(const void* data, size_t len);
    
    // 发送字符串
    void send_string(const std::string& data);
    
    // 发送 TLV 消息
    void send_tlv(const uint8_t* tlv, size_t tlv_len);
    
    // 关闭连接
    void close();
    
    // 获取状态
    ConnectionState state() const { return state_; }
    
    // 获取 FD
    int fd() const { return fd_; }
    
    // 获取地址
    std::string address() const { 
        char buf[64];
        // 使用 inet_ntop 的替代实现
        char addr_str[INET_ADDRSTRLEN];
        struct in_addr in;
        memcpy(&in, &addr_.sin_addr, sizeof(in));
        if (inet_ntop(AF_INET, &in, buf, sizeof(buf)) != nullptr) {
            return std::string(buf) + ":" + std::to_string(ntohs(addr_.sin_port));
        }
        return std::string("unknown");
    }
    
    // 获取本地地址
    std::string local_address() const;
    
    // 设置回调
    void set_message_callback(ReadCallback cb) { 
        message_cb_ = std::move(cb); 
    }
    
    void set_close_callback(CloseCallback cb) { 
        close_cb_ = std::move(cb); 
    }
    
    // 统计
    size_t bytes_sent() const { return bytes_sent_; }
    size_t bytes_received() const { return bytes_received_; }

private:
    void handle_read();
    void handle_write();
    void handle_close();
    void handle_error(int error);
    void set_nonblocking(int fd);
    void set_tcp_nodelay(int fd);
    
    int fd_;
    struct sockaddr_in addr_;
    Reactor* reactor_;
    MemoryPool* pool_;
    
    Channel* channel_;
    ConnectionState state_;
    
    // 读写缓冲
    std::vector<char> read_buf_;
    std::vector<char> write_buf_;
    
    // 统计
    size_t bytes_sent_;
    size_t bytes_received_;
    
    // 回调
    ReadCallback message_cb_;
    CloseCallback close_cb_;
};

using ConnectionPtr = std::shared_ptr<Connection>;

} // namespace rpc

#endif // RPC_CONNECTION_H
