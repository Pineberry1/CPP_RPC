#include "rpc/rpc_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "rpc/json.hpp"

using json = nlohmann::json;

int main() {
    std::cout << "=== RPC Client Demo ===" << std::endl;
    
    // 创建客户端配置
    rpc::ClientConfig client_config;
    client_config.io_threads = 2;
    
    // 创建客户端
    rpc::RpcClient client(client_config);
    
    // 启动客户端
    if (!client.start()) {
        std::cerr << "Failed to start client" << std::endl;
        return 1;
    }
    
    // 连接服务器
    if (!client.connect("127.0.0.1", 9000)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    // 示例 1: 调用加法服务
    std::cout << "\n--- Test AddService ---" << std::endl;
    
    json params1;
    params1["a"] = 10;
    params1["b"] = 20;
    
    auto response1 = client.call("AddService", "add", params1.dump(), 5000);
    
    if (response1.success) {
        std::cout << "10 + 20 = " << json::parse(response1.result)["result"] << std::endl;
    } else {
        std::cout << "Error: " << response1.error << std::endl;
    }
    
    // 示例 2: 调用字符串服务 - 转大写
    std::cout << "\n--- Test StringService (uppercase) ---" << std::endl;
    
    json params2;
    params2["str"] = "hello world";
    params2["action"] = "upper";
    
    auto response2 = client.call("StringService", "transform", params2.dump(), 5000);
    
    if (response2.success) {
        std::cout << "UPPER: " << json::parse(response2.result)["result"] << std::endl;
    } else {
        std::cout << "Error: " << response2.error << std::endl;
    }
    
    // 示例 3: 调用字符串服务 - 转小写
    std::cout << "\n--- Test StringService (lowercase) ---" << std::endl;
    
    json params3;
    params3["str"] = "HELLO WORLD";
    params3["action"] = "lower";
    
    auto response3 = client.call("StringService", "transform", params3.dump(), 5000);
    
    if (response3.success) {
        std::cout << "lower: " << json::parse(response3.result)["result"] << std::endl;
    } else {
        std::cout << "Error: " << response3.error << std::endl;
    }
    
    // 示例 4: 调用字符串服务 - 反转
    std::cout << "\n--- Test StringService (reverse) ---" << std::endl;
    
    json params4;
    params4["str"] = "abcdefg";
    params4["action"] = "reverse";
    
    auto response4 = client.call("StringService", "transform", params4.dump(), 5000);
    
    if (response4.success) {
        std::cout << "reverse: " << json::parse(response4.result)["result"] << std::endl;
    } else {
        std::cout << "Error: " << response4.error << std::endl;
    }
    
    // 示例 5: 调用查询服务
    std::cout << "\n--- Test QueryService ---" << std::endl;
    
    json params5;
    params5["query"] = "user:12345";
    
    auto response5 = client.call("QueryService", "query", params5.dump(), 5000);
    
    if (response5.success) {
        std::cout << "Query: " << json::parse(response5.result)["result"] << std::endl;
    } else {
        std::cout << "Error: " << response5.error << std::endl;
    }
    
    // 批量调用测试
    std::cout << "\n--- Batch Test (100 requests) ---" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        json batch_params;
        batch_params["a"] = i;
        batch_params["b"] = i * 2;
        
        auto resp = client.call("AddService", "add", batch_params.dump(), 5000);
        if (!resp.success) {
            std::cout << "Batch request " << i << " failed: " << resp.error << std::endl;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "100 requests completed in " << duration.count() << "ms" << std::endl;
    std::cout << "Throughput: " << (100.0 * 1000.0 / duration.count()) << " req/s" << std::endl;
    
    // 停止客户端
    client.stop();
    
    std::cout << "\nAll tests completed!" << std::endl;
    
    return 0;
}
