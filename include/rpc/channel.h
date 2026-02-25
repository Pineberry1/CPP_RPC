#ifndef RPC_CHANNEL_H
#define RPC_CHANNEL_H

#include <sys/epoll.h>
#include <functional>
#include <string>

namespace rpc {

// 事件回调类型
using EventCallback = std::function<void(int events)>;
using ReadCallback = std::function<void(const void* data, size_t len)>;
using WriteCallback = std::function<void()>;
using CloseCallback = std::function<void()>;
using ErrorCallback = std::function<void(int error)>;

class Channel {
public:
    explicit Channel(int fd);
    ~Channel();

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_events(int events) { events_ = events; }

    void set_read_callback(ReadCallback cb) { read_cb_ = std::move(cb); }
    void set_write_callback(WriteCallback cb) { write_cb_ = std::move(cb); }
    void set_close_callback(CloseCallback cb) { close_cb_ = std::move(cb); }
    void set_error_callback(ErrorCallback cb) { error_cb_ = std::move(cb); }

    void handle_event(int revents);
    bool is_none_event() const { return events_ == EPOLLERR; }

private:
    int fd_;
    int events_;
    ReadCallback read_cb_;
    WriteCallback write_cb_;
    CloseCallback close_cb_;
    ErrorCallback error_cb_;
};

} // namespace rpc

#endif // RPC_CHANNEL_H
