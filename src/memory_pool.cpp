#include "rpc/memory_pool.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace rpc {

MemoryPool::MemoryPool(size_t initial_size)
    : initial_size_(initial_size),
      used_memory_(0),
      total_memory_(0),
      block_count_(0),
      memory_(nullptr),
      memory_offset_(0) {
    
    // 初始化空闲链表
    for (size_t i = 0; i < NUM_BLOCK_SIZES; ++i) {
        free_lists_[i] = nullptr;
    }
    
    // 分配初始内存
    memory_ = static_cast<char*>(std::malloc(initial_size_));
    if (!memory_) {
        throw std::runtime_error("Failed to allocate memory pool");
    }
    total_memory_ = initial_size_;
    
    // 预分配一些常用尺寸的块
    allocate_new_block(1024);
    allocate_new_block(4096);
}

MemoryPool::~MemoryPool() {
    if (memory_) {
        std::free(memory_);
    }
}

size_t MemoryPool::get_block_size(size_t size) const {
    for (size_t i = 0; i < NUM_BLOCK_SIZES; ++i) {
        if (size <= BLOCK_SIZES[i]) {
            return BLOCK_SIZES[i];
        }
    }
    return BLOCK_SIZES[NUM_BLOCK_SIZES - 1]; // 返回最大尺寸
}

void MemoryPool::allocate_new_block(size_t block_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 分配新内存块
    void* block = std::malloc(block_size);
    if (!block) {
        std::cerr << "Warning: Failed to allocate block of size " << block_size << std::endl;
        return;
    }
    
    // 初始化块结构
    MemoryBlock* mb = new MemoryBlock{
        block,
        block_size,
        free_lists_[get_block_size(block_size)]
    };
    
    free_lists_[get_block_size(block_size)] = mb;
    block_count_++;
}

void* MemoryPool::allocate(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 获取合适的块大小
    size_t block_size = get_block_size(size);
    int idx = static_cast<int>(std::min(block_size < BLOCK_SIZES[NUM_BLOCK_SIZES-1] 
                                          ? std::lower_bound(BLOCK_SIZES, BLOCK_SIZES + NUM_BLOCK_SIZES, block_size) - BLOCK_SIZES
                                          : NUM_BLOCK_SIZES - 1, 
                          NUM_BLOCK_SIZES - 1));
    
    // 尝试从空闲链表获取
    if (free_lists_[idx]) {
        MemoryBlock* block = free_lists_[idx];
        free_lists_[idx] = block->next;
        used_memory_ += block_size;
        return block->ptr;
    }
    
    // 从主内存分配
    if (memory_offset_ + block_size > total_memory_) {
        // 内存不足，尝试重新分配
        size_t new_size = total_memory_ * 2;
        char* new_memory = static_cast<char*>(std::realloc(memory_, new_size));
        if (!new_memory) {
            std::cerr << "Warning: Memory pool exhausted" << std::endl;
            return nullptr;
        }
        memory_ = new_memory;
        total_memory_ = new_size;
    }
    
    void* ptr = memory_ + memory_offset_;
    memory_offset_ += block_size;
    used_memory_ += block_size;
    
    return ptr;
}

void MemoryPool::deallocate(void* ptr, size_t size) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t block_size = get_block_size(size);
    int idx = static_cast<int>(std::min(block_size < BLOCK_SIZES[NUM_BLOCK_SIZES-1] 
                                         ? std::lower_bound(BLOCK_SIZES, BLOCK_SIZES + NUM_BLOCK_SIZES, block_size) - BLOCK_SIZES
                                         : NUM_BLOCK_SIZES - 1, 
                         NUM_BLOCK_SIZES - 1));
    
    // 添加到空闲链表
    MemoryBlock* block = new MemoryBlock{ptr, block_size, free_lists_[idx]};
    free_lists_[idx] = block;
    used_memory_ -= block_size;
}

} // namespace rpc
