#include "random_generator.h"
#include <Security/Security.h>

namespace utils {
namespace crypto {

class SecureRandom {
public:
    static bool GenerateBytes(uint8_t* buffer, size_t length);
    static bool GenerateIV(uint8_t iv[16]);
    static bool GenerateSalt(uint8_t* salt, size_t length);
};

bool SecureRandom::GenerateBytes(uint8_t* buffer, size_t length) {
    if (!buffer || length == 0) {
        return false;
    }
    return SecRandomCopyBytes(kSecRandomDefault, length, buffer) == errSecSuccess;
}

bool SecureRandom::GenerateIV(uint8_t iv[16]) {
    return GenerateBytes(iv, 16);
}

bool SecureRandom::GenerateSalt(uint8_t* salt, size_t length) {
    return GenerateBytes(salt, length);
}

} // namespace crypto
} // namespace utils