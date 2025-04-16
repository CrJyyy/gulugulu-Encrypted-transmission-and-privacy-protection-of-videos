
#include "sm4.h"

// SM4 算法常量定义
namespace {
    const uint32_t FK[4] = {
        0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC
    };

    const uint32_t CK[32] = {
        0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
        0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
        0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
        0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
        0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
        0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
        0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
        0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
    };

    // 辅助函数
    inline uint32_t load_be(const uint8_t b[4]) {
        return (static_cast<uint32_t>(b[0]) << 24) |
               (static_cast<uint32_t>(b[1]) << 16) |
               (static_cast<uint32_t>(b[2]) << 8)  |
               static_cast<uint32_t>(b[3]);
    }

    inline void store_be(uint32_t v, uint8_t b[4]) {
        b[0] = static_cast<uint8_t>(v >> 24);
        b[1] = static_cast<uint8_t>(v >> 16);
        b[2] = static_cast<uint8_t>(v >> 8);
        b[3] = static_cast<uint8_t>(v);
    }

    inline uint32_t sm4_t(uint32_t x) {
        uint32_t b0 = (x >> 24) & 0xFF;
        uint32_t b1 = (x >> 16) & 0xFF;
        uint32_t b2 = (x >> 8) & 0xFF;
        uint32_t b3 = x & 0xFF;

        // S-box 替换 (具体实现需补全)
        uint8_t sm4_sbox[256] = { 0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7, 0x16, 0xB6, 0x14, 0xC2, 0x28, 0xFB, 0x2C, 0x05,
            0x2B, 0x67, 0x9A, 0x76, 0x2A, 0xBE, 0x04, 0xC3, 0xAA, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
            0x9C, 0x42, 0x50, 0xF4, 0x91, 0xEF, 0x98, 0x7A, 0x33, 0x54, 0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62,
            0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95, 0x80, 0xDF, 0x94, 0xFA, 0x75, 0x8F, 0x3F, 0xA6,
            0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73, 0x17, 0xBA, 0x83, 0x59, 0x3C, 0x19, 0xE6, 0x85, 0x4F, 0xA8,
            0x68, 0x6B, 0x81, 0xB2, 0x71, 0x64, 0xDA, 0x8B, 0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D, 0x35,
            0x1E, 0x24, 0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2, 0x25, 0x22, 0x7C, 0x3B, 0x01, 0x21, 0x78, 0x87,
            0xD4, 0x00, 0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52, 0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4, 0xC8, 0x9E,
            0xEA, 0xBF, 0x8A, 0xD2, 0x40, 0xC7, 0x38, 0xB5, 0xA3, 0xF7, 0xF2, 0xCE, 0xF9, 0x61, 0x15, 0xA1,
            0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55, 0xAD, 0x93, 0x32, 0x30, 0xF5, 0x8C, 0xB1, 0xE3,
            0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60, 0xC0, 0x29, 0x23, 0xAB, 0x0D, 0x53, 0x4E, 0x6F,
            0xD5, 0xDB, 0x37, 0x45, 0xDE, 0xFD, 0x8E, 0x2F, 0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C, 0x5B, 0x51,
            0x8D, 0x1B, 0xAF, 0x92, 0xBB, 0xDD, 0xBC, 0x7F, 0x11, 0xD9, 0x5C, 0x41, 0x1F, 0x10, 0x5A, 0xD8,
            0x0A, 0xC1, 0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD, 0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0,
            0x89, 0x69, 0x97, 0x4A, 0x0C, 0x96, 0x77, 0x7E, 0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E, 0xC6, 0x84,
            0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D, 0x20, 0x79, 0xEE, 0x5F, 0x3E, 0xD7, 0xCB, 0x39, 0x48};
        b0 = sm4_sbox[b0];
        b1 = sm4_sbox[b1];
        b2 = sm4_sbox[b2];
        b3 = sm4_sbox[b3];

        uint32_t y = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        // 线性变换 L'
        return y ^ ((y << 13) | (y >> 19)) ^ ((y << 23) | (y >> 9));
    }
}

/* 密钥扩展 */
bool sm4_init(SM4_CTX* ctx, const uint8_t key[16], int mode) {
    if (!ctx || !key || (mode != 0 && mode != 1)) return false;

    ctx->mode = mode;
    uint32_t k[36];
    
    // 加载初始密钥
    k[0] = load_be(key)     ^ FK[0];
    k[1] = load_be(key+4)   ^ FK[1];
    k[2] = load_be(key+8)   ^ FK[2];
    k[3] = load_be(key+12)  ^ FK[3];

    // 生成轮密钥
    for (int i = 0; i < 32; ++i) {
        k[i+4] = k[i] ^ sm4_t(k[i+1] ^ k[i+2] ^ k[i+3] ^ CK[i]);
        ctx->rk_enc[i] = k[i+4];
        ctx->rk_dec[31 - i] = ctx->rk_enc[i];
    }

    memset(ctx->iv, 0, 16);
    return true;
}

/* 单块加密/解密 */
static void sm4_crypt_block(const uint32_t rk[32], 
                           const uint8_t in[16], uint8_t out[16]) {
    if (!rk || !in || !out) return;  //参数检查
    
    uint32_t x[36] = {0}; 
    x[0] = load_be(in);
    x[1] = load_be(in + 4);
    x[2] = load_be(in + 8);
    x[3] = load_be(in + 12);

    for (int i = 0; i < 32; ++i) {
        x[i+4] = x[i] ^ sm4_t(x[i+1] ^ x[i+2] ^ x[i+3] ^ rk[i]);
    }

    store_be(x[35], out);
    store_be(x[34], out + 4);
    store_be(x[33], out + 8);
    store_be(x[32], out + 12);
}


/* CBC 模式 */
void sm4_crypt_cbc(SM4_CTX* ctx, int encrypt,
    const uint8_t* in, uint8_t* out, size_t len) {
    if (!ctx || !in || !out || len % 16 != 0) return;

    const uint32_t* rk = encrypt ? ctx->rk_enc : ctx->rk_dec;
    alignas(16) uint8_t ivec[16];
        memcpy(ivec, ctx->iv, 16);

    if (encrypt) {
        for (size_t i = 0; i < len; i += 16) {
            for (int j = 0; j < 16; ++j) 
                ivec[j] ^= in[i + j];
 
            sm4_crypt_block(rk, ivec, out + i);
            memcpy(ivec, out + i, 16);
            }
    } else {
        alignas(16) uint8_t temp[16];
        for (size_t i = 0; i < len; i += 16) {
            memcpy(temp, in + i, 16);
            sm4_crypt_block(rk, in + i, out + i);

        for (int j = 0; j < 16; ++j)
            out[i + j] ^= ivec[j];
 
        memcpy(ivec, temp, 16);
        }
    }

    memcpy(ctx->iv, ivec, 16);
}