#include "net/rsa.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    printf("=== Test Suite: RSA 512-bit Bignum ModPow ===\n");

    uint8_t plain[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    uint8_t cipher_out[128];
    memset(cipher_out, 0, sizeof(cipher_out));

    size_t out_len = rsa_encrypt(plain, sizeof(plain), cipher_out, sizeof(cipher_out));
    printf("[INFO] Encrypted %zu bytes -> %zu cipher bytes\n", sizeof(plain), out_len);
    assert(out_len == 64);

    // Golden expected cipher hex:
    // 55503dbd4a9a557e0a8f4596f4cbcd31a8ecd553e818d77f12a54da1a6d7541184750c52af6c35a9315aa6465cd9065db4b7d717eab60b9f3c0d43ed89f3cf74
    const uint8_t expected[64] = {
        0x55, 0x50, 0x3d, 0xbd, 0x4a, 0x9a, 0x55, 0x7e, 0x0a, 0x8f, 0x45, 0x96, 0xf4, 0xcb, 0xcd, 0x31,
        0xa8, 0xec, 0xd5, 0x53, 0xe8, 0x18, 0xd7, 0x7f, 0x12, 0xa5, 0x4d, 0xa1, 0xa6, 0xd7, 0x54, 0x11,
        0x84, 0x75, 0x0c, 0x52, 0xaf, 0x6c, 0x35, 0xa9, 0x31, 0x5a, 0xa6, 0x46, 0x5c, 0xd9, 0x06, 0x5d,
        0xb4, 0xb7, 0xd7, 0x17, 0xea, 0xb6, 0x0b, 0x9f, 0x3c, 0x0d, 0x43, 0xed, 0x89, 0xf3, 0xcf, 0x74
    };

    for (int i = 0; i < 64; i++) {
        if (cipher_out[i] != expected[i]) {
            printf("[FAIL] Byte %d mismatch: got 0x%02X, expected 0x%02X\n", i, cipher_out[i], expected[i]);
            assert(cipher_out[i] == expected[i]);
        }
    }

    printf("[PASS] RSA 512-bit modular exponentiation verified with 100%% precision against reference!\n");
    return 0;
}
