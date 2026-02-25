#ifndef RPC_HASH_RING_H
#define RPC_HASH_RING_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>
#include <mutex>

namespace rpc {

// 一致性哈希环
class ConsistentHashRing {
public:
    explicit ConsistentHashRing(int virtual_nodes = 100);
    
    // 添加节点
    void add_node(const std::string& node_id);
    
    // 移除节点
    void remove_node(const std::string& node_id);
    
    // 获取节点
    std::string get_node(const std::string& key) const;
    
    // 获取所有节点
    std::vector<std::string> get_all_nodes() const;
    
    // 节点数量
    size_t node_count() const;
    
    // 计算哈希值
    static uint64_t hash(const std::string& key);
    
private:
    // 虚拟节点 ID 生成器
    std::string make_virtual_node_id(const std::string& node_id, int index) const;
    
    mutable std::mutex mutex_;
    std::map<uint64_t, std::string> ring_; // 哈希值 -> 节点 ID
    std::vector<std::string> nodes_;       // 实际节点列表
    int virtual_nodes_;                    // 虚拟节点数
};

} // namespace rpc

#endif // RPC_HASH_RING_H
