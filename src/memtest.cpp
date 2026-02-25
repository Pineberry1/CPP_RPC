#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "=== Memory Test ===" << std::endl;
    
    std::cout << "Testing malloc..." << std::endl;
    void* ptr = std::malloc(1024 * 1024 * 100); // 100MB
    if (!ptr) {
        std::cerr << "malloc failed!" << std::endl;
        return 1;
    }
    std::cout << "malloc(100MB) succeeded at " << ptr << std::endl;
    
    std::cout << "Testing realloc..." << std::endl;
    void* ptr2 = std::realloc(ptr, 1024 * 1024 * 200);
    if (!ptr2) {
        std::cerr << "realloc failed!" << std::endl;
        return 1;
    }
    std::cout << "realloc succeeded at " << ptr2 << std::endl;
    
    std::cout << "All memory tests passed!" << std::endl;
    std::free(ptr2);
    
    return 0;
}
