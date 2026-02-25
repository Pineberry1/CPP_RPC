#include <iostream>
#include <cstdlib>
int main() {
    void* p = malloc(100*1024*1024);
    std::cout << " malloc: \ << (void*)p << std::endl;
 return 0;
}
