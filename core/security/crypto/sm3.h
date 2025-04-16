#ifndef SM3_H
#define SM3_H

#include <string.h> 
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// SM3上下文结构体（存储中间状态）
typedef struct {
    uint32_t digest[8];      // 哈希中间值
    uint64_t bit_count;      // 已处理的比特数
    uint8_t buffer[64];      // 数据缓冲区
} SM3_CTX;

static const uint32_t IV[8] = {
    0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
    0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E
};

// 初始化上下文
void sm3_init(SM3_CTX* ctx);

// 追加数据到哈希计算
void sm3_update(SM3_CTX* ctx, const uint8_t* data, size_t len);

// 完成哈希计算，输出最终结果
void sm3_final(SM3_CTX* ctx, uint8_t digest[32]);

//压缩函数声明
void sm3_compress(SM3_CTX* ctx);

#ifdef __cplusplus
}
#endif

#endif