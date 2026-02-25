#include "rpc/reactor.h"
#include "rpc/channel.h"
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <cstring>

namespace rpc {

Reactor::Reactor(int id, MemoryPool* pool)
    : id_(id),
      pool_(pool),
      running_(false),
      epoll_fd_(-1) {
    
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

Reactor::~Reactor() {
    if (epoll_fd_ >= 0) {
        close(epoll_fd_);
    }
    
    stop();
}

void Reactor::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    thread_ = std::thread(&Reactor::event_loop, this);
}

void Reactor::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // 唤醒事件循环
    if (epoll_fd_ >= 0) {
        char dummy = 0;
        ::write(epoll_fd_, &dummy, 1);
    }
    
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Reactor::add_channel(Channel* channel) {
    if (!channel || !epoll_fd_) {
        return;
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // 水平触发 + 边缘触发
    ev.data.ptr = channel;
    
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, channel->fd(), &ev) < 0) {
        std::cerr << "Failed to add channel to epoll: " << strerror(errno) << std::endl;
    }
}

void Reactor::update_channel(Channel* channel) {
    if (!channel || !epoll_fd_) {
        return;
    }
    
    struct epoll_event ev;
    ev.events = channel->events();
    ev.data.ptr = channel;
    
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, channel->fd(), &ev) < 0) {
        std::cerr << "Failed to update channel in epoll: " << strerror(errno) << std::endl;
    }
}

void Reactor::delete_channel(Channel* channel) {
    if (!channel || !epoll_fd_) {
        return;
    }
    
    struct epoll_event ev;
    ev.events = 0;
    ev.data.ptr = nullptr;
    
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, channel->fd(), &ev);
}

void Reactor::event_loop() {
    // 初始化事件缓冲
    events_.resize(MAX_EVENTS);
    
    while (running_) {
        handle_events();
    }
}

void Reactor::handle_events() {
    int nfd = epoll_wait(epoll_fd_, events_.data(), MAX_EVENTS, 100); // 100ms 超时
    
    if (nfd < 0) {
        if (errno != EINTR) {
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
        }
        return;
    }
    
    for (int i = 0; i < nfd; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        if (channel) {
            channel->handle_event(events_[i].events);
        }
    }
}

// ReactorPool 实现
ReactorPool::ReactorPool(int io_threads, MemoryPool* pool)
    : next_reactor_idx_(0),
      running_(false) {
    
    for (int i = 0; i < io_threads; ++i) {
        reactors_.push_back(std::make_unique<Reactor>(i, pool));
    }
}

ReactorPool::~ReactorPool() {
    stop();
}

void ReactorPool::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    for (auto& reactor : reactors_) {
        reactor->start();
    }
}

void ReactorPool::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    for (auto& reactor : reactors_) {
        reactor->stop();
    }
}

Reactor* ReactorPool::next_reactor() {
    int idx = next_reactor_idx_++ % reactors_.size();
    return reactors_[idx].get();
}

Reactor* ReactorPool::get_reactor(int id) const {
    if (id >= 0 && id < static_cast<int>(reactors_.size())) {
        return reactors_[id].get();
    }
    return nullptr;
}

int ReactorPool::select_reactor() {
    return next_reactor_idx_++ % reactors_.size();
}

} // namespace rpc
