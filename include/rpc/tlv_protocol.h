#ifndef RPC_TLV_PROTOCOL_H
#define RPC_TLV_PROTOCOL_H

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include "rpc_types.h"

namespace rpc {

// TLV 协议头结构 (16 字节)
struct __attribute__((packed)) TlvHeader {
    uint8_t version;          // 协议版本
    uint8_t type;             // 消息类型 (Request/Response/Heartbeat/Error)
    uint16_t length;          // 数据部分长度
    uint64_t request_id;      // 请求 ID
};

// TLV 协议常量
constexpr size_t HEADER_SIZE = sizeof(TlvHeader);
constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB 最大消息
constexpr uint16_t TLV_MAGIC = 0x5250; // "RP"

class TlvProtocol {
public:
    // 编码请求
    static std::vector<uint8_t> encode_request(const RpcRequest& request, uint64_t request_id);
    
    // 编码响应
    static std::vector<uint8_t> encode_response(const RpcResponse& response, uint64_t request_id);
    
    // 编码心跳
    static std::vector<uint8_t> encode_heartbeat();
    
    // 解码请求
    static bool decode_request(const uint8_t* data, size_t len, RpcRequest& request);
    
    // 解码响应
    static bool decode_response(const uint8_t* data, size_t len, RpcResponse& response);
    
    // 解码心跳
    static bool decode_heartbeat(const uint8_t* data, size_t len);
    
    // 验证头
    static bool validate_header(const uint8_t* data);
    
    // 获取头大小
    static size_t header_size() { return HEADER_SIZE; }
    
    // 处理粘包/半包
    static std::vector<std::pair<const uint8_t*, size_t>> split_messages(
        const uint8_t* buffer, size_t buffer_len);
};

} // namespace rpc

#endif // RPC_TLV_PROTOCOL_H
