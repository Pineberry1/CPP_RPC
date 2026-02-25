#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>

// 最小化的内存池测试
class MinMemPool {
public:
    MinMemPool(size_t size) : total_size_(size) {
        std::cout << "Creating mempool, size=" << size << std::endl;
        memory_ = new char[size];
        std::cout << "Mempool allocated: " << (void*)memory_ << std::endl;
    }
    
    ~MinMemPool() {
        std::cout << "Destroying mempool" << std::endl;
        delete[] memory_;
    }
    
    void* allocate(size_t size) {
        void* ptr = memory_ + offset_;
        offset_ += size;
        return ptr;
    }
    
private:
    char* memory_;
    size_t total_size_;
    size_t offset_ = 0;
};

// 最小化的 Reactor 测试
class MinReactor {
public:
    MinReactor(int id, MinMemPool* pool) : id_(id), pool_(pool) {
        std::cout << "Creating reactor " << id_ << std::endl;
    }
    
    ~MinReactor() {
        std::cout << "Destroying reactor " << id_ << std::endl;
    }
    
    void start() {
        running_ = true;
        thread_ = std::thread(&MinReactor::loop, this);
        std::cout << "Reactor " << id_ << " started" << std::endl;
    }
    
    void stop() {
        running_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
        std::cout << "Reactor " << id_ << " stopped" << std::endl;
    }
    
private:
    void loop() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    int id_;
    MinMemPool* pool_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

class MinReactorPool {
public:
    MinReactorPool(int count, MinMemPool* pool) : pool_(pool) {
        std::cout << "Creating reactor pool with " << count << " reactors" << std::endl;
        for (int i = 0; i < count; i++) {
            reactors_.push_back(std::make_unique<MinReactor>(i, pool_));
        }
    }
    
    ~MinReactorPool() {
        stop();
        std::cout << "Destroying reactor pool" << std::endl;
    }
    
    void start() {
        for (auto& r : reactors_) {
            r->start();
        }
    }
    
    void stop() {
        for (auto& r : reactors_) {
            r->stop();
        }
    }
    
private:
    std::vector<std::unique_ptr<MinReactor>> reactors_;
    MinMemPool* pool_;
};

int main() {
    std::cout << "=== Minimal Test ===" << std::endl;
    
    std::cout << "\nStep 1: Creating memory pool..." << std::endl;
    MinMemPool pool(50 * 1024 * 1024); // 50MB
    
    std::cout << "\nStep 2: Creating reactor pool..." << std::endl;
    MinReactorPool reactor_pool(2, &pool);
    
    std::cout << "\nStep 3: Starting reactor pool..." << std::endl;
    reactor_pool.start();
    
    std::cout << "\nStep 4: Waiting 3 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "\nStep 5: Stopping reactor pool..." << std::endl;
    reactor_pool.stop();
    
    std::cout << "\n=== Test Completed Successfully! ===" << std::endl;
    
    return 0;
}
