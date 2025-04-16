//随机数生成

#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H

#include <cstdint>
#include <cstddef>

namespace utils {
    class SecureRandom {
    public:
        static bool GenerateSecureRandom(uint8_t* buffer, size_t length);
        static bool GenerateIV(uint8_t iv[16]);
    private:
        static bool InitializeSecureRandom();
    };
}

#endif