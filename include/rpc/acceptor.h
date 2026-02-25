#ifndef RPC_ACCEPTOR_H
#define RPC_ACCEPTOR_H

#include "channel.h"
#include "reactor.h"
#include "rpc_types.h"
#include <functional>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

namespace rpc {

// 连接回调
using ConnectionCallback = std::function<void(int fd, const struct sockaddr_in& addr)>;

class Acceptor {
public:
    explicit Acceptor(Reactor* reactor, const ConnectionConfig& config);
    ~Acceptor();

    // 启动监听
    bool listen(const std::string& bind_ip, uint16_t port);
    
    // 停止监听
    void stop();
    
    // 是否运行中
    bool is_listening() const { return listening_; }
    
    // 设置连接回调
    void set_connection_callback(ConnectionCallback cb) { 
        connection_cb_ = std::move(cb); 
    }
    
    int fd() const { return fd_; }

private:
    void handle_accept();

    Reactor* reactor_;
    ConnectionConfig config_;
    
    int fd_;
    Channel* channel_;
    
    std::atomic<bool> listening_;
    ConnectionCallback connection_cb_;
};

} // namespace rpc

#endif // RPC_ACCEPTOR_H
