#include "rpc/rpc_types.h"

namespace rpc {

void ServiceRegistry::register_service(const std::string& name, RpcHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[name] = handler;
}

RpcHandler ServiceRegistry::get_handler(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = handlers_.find(name);
    if (it != handlers_.end()) {
        return it->second;
    }
    return nullptr;
}

size_t ServiceRegistry::service_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return handlers_.size();
}

} // namespace rpc
