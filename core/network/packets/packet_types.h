//数据包类型定义


#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H

#include <cstdint>


#pragma pack(push, 1)
struct PacketHeader {
    uint32_t session_id;
    uint32_t seq_num;
    uint16_t fragment_id;      // 分片序号
    uint16_t total_fragments;  // 总分片数
    uint16_t payload_len;
    uint8_t iv[16];         
    uint8_t sm3_digest[32]; 
};
#pragma pack(pop)


#endif 