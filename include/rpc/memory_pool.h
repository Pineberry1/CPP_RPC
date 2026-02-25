#ifndef RPC_MEMORY_POOL_H
#define RPC_MEMORY_POOL_H

#include <cstddef>
#include <vector>
#include <mutex>
#include <cstdint>

namespace rpc {

// 内存块大小配置
constexpr size_t BLOCK_SIZES[] = {
    64, 128, 256, 512, 1024, 2048, 4096, 8192,
    16384, 32768, 65536, 131072, 262144, 524288, 1048576
};
constexpr size_t NUM_BLOCK_SIZES = sizeof(BLOCK_SIZES) / sizeof(BLOCK_SIZES[0]);

class MemoryPool {
public:
    explicit MemoryPool(size_t initial_size = 1024 * 1024 * 100); // 默认 100MB
    ~MemoryPool();

    // 分配内存
    void* allocate(size_t size);
    
    // 释放内存
    void deallocate(void* ptr, size_t size);
    
    // 统计信息
    size_t used_memory() const { return used_memory_; }
    size_t total_memory() const { return total_memory_; }
    size_t block_count() const { return block_count_; }

private:
    // 内存块结构
    struct MemoryBlock {
        void* ptr;
        size_t size;
        MemoryBlock* next;
    };

    // 获取合适的块大小
    size_t get_block_size(size_t size) const;
    
    // 分配新的内存块
    void allocate_new_block(size_t block_size);

    // 内存池配置
    size_t initial_size_;
    
    // 当前统计
    size_t used_memory_;
    size_t total_memory_;
    size_t block_count_;
    
    // 各尺寸内存块链表
    MemoryBlock* free_lists_[NUM_BLOCK_SIZES];
    
    // 线程锁
    mutable std::mutex mutex_;
    
    // 原始内存指针
    char* memory_;
    size_t memory_offset_;
};

} // namespace rpc

#endif // RPC_MEMORY_POOL_H
