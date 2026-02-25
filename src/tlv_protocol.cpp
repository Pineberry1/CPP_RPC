#include "rpc/tlv_protocol.h"
#include "rpc/rpc_types.h"
#include <iostream>

namespace rpc {

std::vector<uint8_t> TlvProtocol::encode_request(const RpcRequest& request, uint64_t request_id) {
    // 编码 JSON 参数
    std::string json = "{\"service\":\"" + request.service_name + 
                       "\",\"method\":\"" + request.method_name + 
                       "\",\"params\":" + request.params + 
                       ",\"caller\":\"" + request.caller_id + "\"}";
    
    size_t data_len = json.size();
    if (data_len > MAX_MESSAGE_SIZE - HEADER_SIZE) {
        throw std::runtime_error("Request too large");
    }
    
    std::vector<uint8_t> buffer(HEADER_SIZE + data_len);
    
    // 填充头
    TlvHeader* header = reinterpret_cast<TlvHeader*>(buffer.data());
    header->version = PROTOCOL_VERSION;
    header->type = static_cast<uint8_t>(MessageType::REQUEST);
    header->length = static_cast<uint16_t>(data_len);
    header->request_id = request_id;
    
    // 填充数据
    std::memcpy(buffer.data() + HEADER_SIZE, json.c_str(), data_len);
    
    return buffer;
}

std::vector<uint8_t> TlvProtocol::encode_response(const RpcResponse& response, uint64_t request_id) {
    // 编码 JSON 结果
    std::string json = "{\"success\":" + std::string(response.success ? "true" : "false") +
                       ",\"result\":\"" + response.result +
                       "\",\"error\":\"" + response.error + "\"}";
    
    size_t data_len = json.size();
    if (data_len > MAX_MESSAGE_SIZE - HEADER_SIZE) {
        throw std::runtime_error("Response too large");
    }
    
    std::vector<uint8_t> buffer(HEADER_SIZE + data_len);
    
    // 填充头
    TlvHeader* header = reinterpret_cast<TlvHeader*>(buffer.data());
    header->version = PROTOCOL_VERSION;
    header->type = static_cast<uint8_t>(MessageType::RESPONSE);
    header->length = static_cast<uint16_t>(data_len);
    header->request_id = request_id;
    
    // 填充数据
    std::memcpy(buffer.data() + HEADER_SIZE, json.c_str(), data_len);
    
    return buffer;
}

std::vector<uint8_t> TlvProtocol::encode_heartbeat() {
    std::vector<uint8_t> buffer(HEADER_SIZE);
    
    TlvHeader* header = reinterpret_cast<TlvHeader*>(buffer.data());
    header->version = PROTOCOL_VERSION;
    header->type = static_cast<uint8_t>(MessageType::HEARTBEAT);
    header->length = 0;
    header->request_id = 0;
    
    return buffer;
}

bool TlvProtocol::decode_request(const uint8_t* data, size_t len, RpcRequest& request) {
    if (len < HEADER_SIZE || !validate_header(data)) {
        return false;
    }
    
    const TlvHeader* header = reinterpret_cast<const TlvHeader*>(data);
    if (header->type != static_cast<uint8_t>(MessageType::REQUEST)) {
        return false;
    }
    
    size_t data_len = header->length;
    if (HEADER_SIZE + data_len > len) {
        return false;
    }
    
    // 解析 JSON
    const char* json_str = reinterpret_cast<const char*>(data + HEADER_SIZE);
    std::string json(json_str, data_len);
    
    // 简单解析 (实际项目应该用 JSON 库)
    size_t pos = json.find("\"service\":\"");
    if (pos != std::string::npos) {
        pos += 11;
        size_t end = json.find("\"", pos);
        request.service_name = json.substr(pos, end - pos);
    }
    
    pos = json.find("\"method\":\"");
    if (pos != std::string::npos) {
        pos += 10;
        size_t end = json.find("\"", pos);
        request.method_name = json.substr(pos, end - pos);
    }
    
    pos = json.find("\"params\":");
    if (pos != std::string::npos) {
        pos += 8;
        size_t end = json.find("}", pos);
        request.params = json.substr(pos, end - pos);
    }
    
    pos = json.find("\"caller\":\"");
    if (pos != std::string::npos) {
        pos += 9;
        size_t end = json.find("\"", pos);
        request.caller_id = json.substr(pos, end - pos);
    }
    
    request.request_id = header->request_id;
    return true;
}

bool TlvProtocol::decode_response(const uint8_t* data, size_t len, RpcResponse& response) {
    if (len < HEADER_SIZE || !validate_header(data)) {
        return false;
    }
    
    const TlvHeader* header = reinterpret_cast<const TlvHeader*>(data);
    if (header->type != static_cast<uint8_t>(MessageType::RESPONSE)) {
        return false;
    }
    
    size_t data_len = header->length;
    if (HEADER_SIZE + data_len > len) {
        return false;
    }
    
    // 解析 JSON
    const char* json_str = reinterpret_cast<const char*>(data + HEADER_SIZE);
    std::string json(json_str, data_len);
    
    // 简单解析
    if (json.find("\"success\":true") != std::string::npos) {
        response.success = true;
    } else if (json.find("\"success\":false") != std::string::npos) {
        response.success = false;
    }
    
    size_t pos = json.find("\"result\":\"");
    if (pos != std::string::npos) {
        pos += 10;
        size_t end = json.find("\"", pos);
        response.result = json.substr(pos, end - pos);
    }
    
    pos = json.find("\"error\":\"");
    if (pos != std::string::npos) {
        pos += 9;
        size_t end = json.find("\"", pos);
        response.error = json.substr(pos, end - pos);
    }
    
    response.request_id = header->request_id;
    return true;
}

bool TlvProtocol::decode_heartbeat(const uint8_t* data, size_t len) {
    if (len < HEADER_SIZE || !validate_header(data)) {
        return false;
    }
    
    const TlvHeader* header = reinterpret_cast<const TlvHeader*>(data);
    return header->type == static_cast<uint8_t>(MessageType::HEARTBEAT);
}

bool TlvProtocol::validate_header(const uint8_t* data) {
    if (!data) return false;
    
    const TlvHeader* header = reinterpret_cast<const TlvHeader*>(data);
    if (header->version != PROTOCOL_VERSION) {
        return false;
    }
    
    if (header->length > MAX_MESSAGE_SIZE) {
        return false;
    }
    
    return true;
}

std::vector<std::pair<const uint8_t*, size_t>> TlvProtocol::split_messages(
    const uint8_t* buffer, size_t buffer_len) {
    
    std::vector<std::pair<const uint8_t*, size_t>> messages;
    
    size_t offset = 0;
    while (offset + HEADER_SIZE <= buffer_len) {
        // 读取头
        const TlvHeader* header = reinterpret_cast<const TlvHeader*>(buffer + offset);
        
        // 检查数据是否完整
        if (offset + HEADER_SIZE + header->length > buffer_len) {
            break;
        }
        
        // 验证头
        if (!validate_header(buffer + offset)) {
            offset++;
            continue;
        }
        
        // 提取消息
        messages.emplace_back(buffer + offset, HEADER_SIZE + header->length);
        offset += HEADER_SIZE + header->length;
    }
    
    return messages;
}

} // namespace rpc
