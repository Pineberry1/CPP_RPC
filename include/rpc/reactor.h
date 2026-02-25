#ifndef RPC_REACTOR_H
#define RPC_REACTOR_H

#include "channel.h"
#include "memory_pool.h"
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <unistd.h>
#include <sys/epoll.h>

namespace rpc {

// Reactor 处理器
class Reactor {
public:
    explicit Reactor(int id, MemoryPool* pool);
    ~Reactor();

    // 启动 Reactor 线程
    void start();
    
    // 停止 Reactor 线程
    void stop();
    
    // 是否运行中
    bool is_running() const { return running_; }
    
    // 添加通道
    void add_channel(Channel* channel);
    
    // 更新通道事件
    void update_channel(Channel* channel);
    
    // 删除通道
    void delete_channel(Channel* channel);
    
    // 获取 ID
    int id() const { return id_; }
    
    // 获取内存池
    MemoryPool* memory_pool() const { return pool_; }
    
    // 获取 epoll fd
    int epoll_fd() const { return epoll_fd_; }

private:
    void event_loop();
    void handle_events();

    int id_;
    MemoryPool* pool_;
    
    std::atomic<bool> running_;
    std::thread thread_;
    
    int epoll_fd_;
    std::vector<Channel*> channels_;
    
    // 事件缓冲（在堆上，避免堆栈溢出）
    std::vector<struct epoll_event> events_;
    static constexpr int MAX_EVENTS = 65536;
};

// Reactor 线程池
class ReactorPool {
public:
    explicit ReactorPool(int io_threads, MemoryPool* pool);
    ~ReactorPool();
    
    void start();
    void stop();
    
    // 获取下一个 Reactor (轮询)
    Reactor* next_reactor();
    
    // 获取指定 ID 的 Reactor
    Reactor* get_reactor(int id) const;
    
    int reactor_count() const { return reactors_.size(); }

private:
    int select_reactor();

    std::vector<std::unique_ptr<Reactor>> reactors_;
    std::atomic<int> next_reactor_idx_;
    std::atomic<bool> running_;
};

} // namespace rpc

#endif // RPC_REACTOR_H
