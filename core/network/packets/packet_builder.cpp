//数据包构建实现

#include <iostream>
#include "packet_builder.h"
#include "packet_types.h"
#include "../../security/crypto/sm3.h"    
#include "../../security/crypto/sm4.h"    
#include <cstring>
#include <string.h>
#include "../../security/crypto/random_generator.h"
#include "../../network/session/session_manager.h"
#include "../../network/session/session_context.h"

using namespace std;

vector<uint8_t> PacketBuilder::BuildPacket(
    uint32_t session_id,          //当前会话的唯一标识符（由会话管理器分配
    const uint8_t* payload,       //原始负载数据指针（需加密的编码后视频帧）
    size_t payload_len,           //负载数据长度
    const uint8_t sm4_key[16],
    const uint8_t sm3_salt[32])
{
    // 参数检查
    if (!payload || payload_len == 0 || !sm4_key || !sm3_salt) {
        throw std::runtime_error("Invalid parameters");
    }

    // 获取会话信息
    SessionContext* session = SessionManager::GetInstance().GetSession(session_id);
    if (!session || !session->IsValid()) {
        throw std::runtime_error("Invalid session");
    }

    // 生成随机IV
    uint8_t iv[16];
    if (!utils::SecureRandom::GenerateIV(iv)) {
        throw std::runtime_error("Failed to generate IV");
    }

    // SM4-CBC加密
    SM4_CTX ctx_4;
    if (!sm4_init(&ctx_4, sm4_key, 1)) { // 1表示CBC模式
        throw std::runtime_error("Failed to initialize SM4");
    }
    memcpy(ctx_4.iv, iv, 16);
    vector<uint8_t> ciphertext(payload_len);
    sm4_crypt_cbc(&ctx_4, 1, payload, ciphertext.data(), payload_len);
    
    // 计算SM3哈希
    SM3_CTX ctx_3;
    uint8_t digest[32];
    sm3_init(&ctx_3);
    sm3_update(&ctx_3, sm3_salt, 32);
    sm3_update(&ctx_3, iv, 16);
    sm3_update(&ctx_3, ciphertext.data(), ciphertext.size());
    sm3_final(&ctx_3, digest);

    // 构建数据包头
    PacketHeader header;
    header.session_id = htonl(session_id);
    header.seq_num = htonl(session->GetAndIncrementSeq());
    header.payload_len = htons(static_cast<uint16_t>(ciphertext.size()));
    memcpy(header.iv, iv, 16);
    memcpy(header.sm3_digest, digest, 32);

    // 序列化数据包
    vector<uint8_t> packet;
    packet.reserve(sizeof(PacketHeader) + ciphertext.size());
    const uint8_t* header_ptr = reinterpret_cast<const uint8_t*>(&header);
    packet.insert(packet.end(), header_ptr, header_ptr + sizeof(PacketHeader));
    packet.insert(packet.end(), ciphertext.begin(), ciphertext.end());

    return packet;
}

bool PacketBuilder::ParsePacket( 
    const uint8_t* packet_data,
    size_t packet_len,
    PacketHeader& header,
    vector<uint8_t>& decrypted_payload,
    const uint8_t sm4_key[16],
    const uint8_t sm3_salt[32]
) {
    if (packet_len < sizeof(PacketHeader)) {
        return false; //数据包长度不足
    }

    //解析包头
    memcpy(&header, packet_data, sizeof(PacketHeader));
    header.session_id = ntohl(header.session_id);
    header.seq_num = ntohl(header.seq_num);
    header.payload_len = ntohs(header.payload_len);

    // 确保session有效性检查
    SessionContext* session = SessionManager::GetInstance().GetSession(header.session_id);
    if (!session || !session->IsValid()) {
        throw runtime_error("无效或过期的会话");
    }

    //检查数据包长度
    if (packet_len != sizeof(PacketHeader) + header.payload_len) {
        return false; // 数据包长度不匹配
    }

    //提取IV和密文
    uint8_t iv[16];
    memcpy(iv, header.iv, 16);
    
    const uint8_t* ciphertext = packet_data + sizeof(PacketHeader);
    
    //SM4-CBC解密
    SM4_CTX ctx_4;
    if (!sm4_init(&ctx_4, sm4_key, 1)) {
        return false;
    }
    memcpy(ctx_4.iv, iv, 16);
    decrypted_payload.resize(header.payload_len);
    sm4_crypt_cbc(&ctx_4, 0, ciphertext, decrypted_payload.data(), header.payload_len);

    //计算SM3哈希值并验证完整性
    SM3_CTX ctx;
    sm3_init(&ctx);
    sm3_update(&ctx, sm3_salt, 32);                             // 添加盐值防预计算攻击
    sm3_update(&ctx, iv, 16);                                   // 包含IV确保哈希与加密绑定
    sm3_update(&ctx, ciphertext, header.payload_len);           // 实际加密数据

    uint8_t digest[32];
    sm3_final(&ctx, digest);

    return memcmp(digest, header.sm3_digest, 32) == 0; // 验证哈希值
}