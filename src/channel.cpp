#include "rpc/channel.h"
#include <iostream>

namespace rpc {

Channel::Channel(int fd)
    : fd_(fd),
      events_(EPOLLIN) {
}

Channel::~Channel() {
}

void Channel::handle_event(int revents) {
    if (revents & EPOLLERR) {
        if (error_cb_) {
            error_cb_(EPOLLERR);
        }
        return;
    }
    
    if (revents & (EPOLLIN | EPOLLHUP)) {
        if (read_cb_) {
            read_cb_(nullptr, 0);
        }
    }
    
    if (revents & EPOLLOUT) {
        if (write_cb_) {
            write_cb_();
        }
    }
    
    if (revents & (EPOLLIN | EPOLLHUP | EPOLLERR)) {
        if (close_cb_) {
            close_cb_();
        }
    }
}

} // namespace rpc
