#ifndef SM4_H
#define SM4_H

#include <cstdint>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

// SM4上下文结构体
typedef struct {
    uint32_t rk_enc[32];    // 加密轮密钥
    uint32_t rk_dec[32];    // 解密轮密钥
    uint8_t  iv[16];        // CBC模式初始向量
    int      mode;          // 0: ECB, 1: CBC
} SM4_CTX;

// 初始化上下文
bool sm4_init(SM4_CTX* ctx, const uint8_t key[16], int mode);

// CBC 模式加密/解密
void sm4_crypt_cbc(SM4_CTX* ctx, int encrypt,
                   const uint8_t* in, uint8_t* out, size_t len);

#ifdef __cplusplus
}
#endif

#endif