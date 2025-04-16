
#include "sm3.h"
#include <vector>

using namespace std;


//------------------- 工具函数 -------------------
static inline uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static inline uint32_t P0(uint32_t x) {
    return x ^ rotl32(x, 9) ^ rotl32(x, 17);
}

static inline uint32_t P1(uint32_t x) {
    return x ^ rotl32(x, 15) ^ rotl32(x, 23);
}

static inline uint32_t T(int j) {
    return (j < 16) ? 0x79CC4519 : 0x7A879D8A;
}

static inline uint32_t FF(uint32_t X, uint32_t Y, uint32_t Z, int j) {
    return (j < 16) ? (X ^ Y ^ Z) : ((X & Y) | (X & Z) | (Y & Z));
}

static inline uint32_t GG(uint32_t X, uint32_t Y, uint32_t Z, int j) {
    return (j < 16) ? (X ^ Y ^ Z) : ((X & Y) | (~X & Z));
}

//------------------- 核心算法 -------------------
void sm3_compress(SM3_CTX* ctx) {
    uint32_t W[68] = {0};
    uint32_t W1[64] = {0};

    // 消息扩展
    for (int i = 0; i < 16; ++i) {
        W[i] = (ctx->buffer[i*4] << 24) |
               (ctx->buffer[i*4+1] << 16) |
               (ctx->buffer[i*4+2] << 8) |
               ctx->buffer[i*4+3];
    }

    for (int j = 16; j < 68; ++j) {
        W[j] = P1(W[j-16] ^ W[j-9] ^ rotl32(W[j-3], 15)) ^
               rotl32(W[j-13], 7) ^ W[j-6];
    }

    for (int j = 0; j < 64; ++j) {
        W1[j] = W[j] ^ W[j+4];
    }

    // 压缩函数
    uint32_t A = ctx->digest[0];
    uint32_t B = ctx->digest[1];
    uint32_t C = ctx->digest[2];
    uint32_t D = ctx->digest[3];
    uint32_t E = ctx->digest[4];
    uint32_t F = ctx->digest[5];
    uint32_t G = ctx->digest[6];
    uint32_t H = ctx->digest[7];

    for (int j = 0; j < 64; ++j) {
        uint32_t SS1 = rotl32(rotl32(A, 12) + E + rotl32(T(j), j), 7);
        uint32_t SS2 = SS1 ^ rotl32(A, 12);
        uint32_t TT1 = FF(A, B, C, j) + D + SS2 + W1[j];
        uint32_t TT2 = GG(E, F, G, j) + H + SS1 + W[j];
        
        D = C;
        C = rotl32(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rotl32(F, 19);
        F = E;
        E = P0(TT2);
    }

    ctx->digest[0] ^= A;
    ctx->digest[1] ^= B;
    ctx->digest[2] ^= C;
    ctx->digest[3] ^= D;
    ctx->digest[4] ^= E;
    ctx->digest[5] ^= F;
    ctx->digest[6] ^= G;
    ctx->digest[7] ^= H;
}

//------------------- 接口函数 -------------------
void sm3_init(SM3_CTX* ctx) {
    memcpy(ctx->digest, IV, sizeof(IV));
    ctx->bit_count = 0;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

void sm3_update(SM3_CTX* ctx, const uint8_t* data, size_t len) {
    uint32_t index = (ctx->bit_count >> 3) % 64;
    ctx->bit_count += len << 3;

    while (len > 0) {
        uint32_t copy_len = 64 - index;
        if (copy_len > len) copy_len = len;

        memcpy(&ctx->buffer[index], data, copy_len);
        
        if (copy_len < 64) {
            index += copy_len;
            break;
        }
        
        sm3_compress(ctx);
        index = 0;
        len -= copy_len;
        data += copy_len;
    }
}

void sm3_final(SM3_CTX* ctx, uint8_t digest[32]) {
    uint32_t index = (ctx->bit_count >> 3) % 64;
    uint64_t bit_len = ctx->bit_count;

    ctx->buffer[index++] = 0x80;

    if (index > 56) {
        memset(&ctx->buffer[index], 0, 64 - index);
        sm3_compress(ctx);
        index = 0;
    }

    uint64_t be_len = __builtin_bswap64(bit_len);
    memcpy(ctx->buffer + 56, &be_len, 8);
    sm3_compress(ctx);

    for (int i = 0; i < 8; ++i) {
        digest[i*4]   = (ctx->digest[i] >> 24) & 0xFF;
        digest[i*4+1] = (ctx->digest[i] >> 16) & 0xFF;
        digest[i*4+2] = (ctx->digest[i] >> 8)  & 0xFF;
        digest[i*4+3] = ctx->digest[i]         & 0xFF;
    }

    memset(ctx, 0, sizeof(SM3_CTX));
}