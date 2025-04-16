#ifndef SM9_H
#define SM9_H

#include <stdint.h>
#include <stdlib.h>

#include <openssl/bn.h> // For BIGNUM type

 //椭圆曲线参数
typedef struct {
    BIGNUM *p;     // 素数域
    BIGNUM *a, *b; // 曲线方程参数
    BIGNUM *n;     // 曲线阶数
    BIGNUM *h;     // 余因子
    BIGNUM *gx, *gy; // 基点坐标
} SM9_EC_PARAMS;


//签名结构体
typedef struct {
    uint8_t r[32];         // 签名值R
    uint8_t s[32];         // 签名值S
} SM9_SIGNATURE;

//主密钥对
typedef struct {
    uint64_t master_priv;  // 主公钥参数 (实际应为大整数)
    SM9_EC_PARAMS* params; // 曲线参数
} SM9_MASTER_KEY;
void sm9_generate_master_key(SM9_MASTER_KEY* master_key);



typedef struct {
    uint8_t id[64];        // 用户标识 (如邮箱)
    uint64_t priv_key[4];  // 用户私钥 (大整数存储)
} SM9_USER_KEY;

//初始化SM9系统 (加载曲线参数)
void sm9_init(SM9_EC_PARAMS* params);
//签名函数
void sm9_sign(const SM9_USER_KEY* user_key,
             const uint8_t* msg, size_t msg_len,
             SM9_SIGNATURE* sig);

//验证签名
int sm9_verify(const SM9_MASTER_KEY* master,
              const uint8_t* id,
              const uint8_t* msg, size_t msg_len,
              const SM9_SIGNATURE* sig);


//为用户生成私钥
void sm9_generate_user_key(const SM9_MASTER_KEY* master, 
    const uint8_t* id, 
    SM9_USER_KEY* user_key);

    void sm9_cleanup_params();
#endif