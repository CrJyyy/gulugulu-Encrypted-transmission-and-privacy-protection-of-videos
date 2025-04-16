//数据包构建类声明

#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H

#include "packet_types.h"
#include <vector>
#include <cstdint>

class PacketBuilder {
public:
    // 构建加密数据包 (返回序列化后的二进制数据)
    static std::vector<uint8_t> BuildPacket(
        uint32_t session_id,
        const uint8_t* payload,     // 原始数据
        size_t payload_len,         // 数据长度
        const uint8_t sm4_key[16],  // SM4密钥
        const uint8_t sm3_salt[32]  // SM3盐值
    );

    bool ParsePacket(const uint8_t *packet_data, size_t packet_len, PacketHeader &header, std::vector<uint8_t> &decrypted_payload, const uint8_t sm4_key[16], const uint8_t sm3_salt[32]);

};

#endif 