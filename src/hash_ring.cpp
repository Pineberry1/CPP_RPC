#include "rpc/hash_ring.h"
#include <functional>
#include <cstring>

namespace rpc {

ConsistentHashRing::ConsistentHashRing(int virtual_nodes)
    : virtual_nodes_(virtual_nodes) {
}

uint64_t ConsistentHashRing::hash(const std::string& key) {
    // 使用 FNV-1a 哈希
    uint64_t hash = 14695981039346656037ULL;
    for (char c : key) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string ConsistentHashRing::make_virtual_node_id(const std::string& node_id, int index) const {
    return node_id + "#VN" + std::to_string(index);
}

void ConsistentHashRing::add_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查节点是否已存在
    auto it = std::find(nodes_.begin(), nodes_.end(), node_id);
    if (it != nodes_.end()) {
        return; // 节点已存在
    }
    
    nodes_.push_back(node_id);
    
    // 添加虚拟节点到环
    for (int i = 0; i < virtual_nodes_; ++i) {
        std::string vn_id = make_virtual_node_id(node_id, i);
        uint64_t hn = hash(vn_id);
        ring_[hn] = node_id;
    }
}

void ConsistentHashRing::remove_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从实际节点列表移除
    nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), node_id), nodes_.end());
    
    // 从环中移除该节点的所有虚拟节点
    for (int i = 0; i < virtual_nodes_; ++i) {
        std::string vn_id = make_virtual_node_id(node_id, i);
        uint64_t hn = hash(vn_id);
        ring_.erase(hn);
    }
}

std::string ConsistentHashRing::get_node(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (ring_.empty()) {
        return "";
    }
    
    uint64_t hn = hash(key);
    
    // 在环中查找大于等于该哈希值的节点
    auto it = ring_.lower_bound(hn);
    if (it == ring_.end()) {
        it = ring_.begin(); // 绕回第一个节点
    }
    
    return it->second;
}

std::vector<std::string> ConsistentHashRing::get_all_nodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_;
}

size_t ConsistentHashRing::node_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.size();
}

} // namespace rpc
