#ifndef RS2_RSA_H
#define RS2_RSA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BIGNUM_MAX_LIMBS 64 // 64 * 32 = 2048 bits max capacity

typedef struct {
    uint32_t limbs[BIGNUM_MAX_LIMBS];
    int length; // number of active limbs
} Bignum;

void bignum_zero(Bignum *n);
void bignum_copy(Bignum *dst, const Bignum *src);
void bignum_from_bytes(Bignum *n, const uint8_t *bytes, size_t len);
void bignum_from_hex(Bignum *n, const char *hex);
size_t bignum_to_java_bytes(const Bignum *n, uint8_t *out_buf, size_t max_out);

int  bignum_cmp(const Bignum *a, const Bignum *b);
void bignum_add(Bignum *res, const Bignum *a, const Bignum *b);
void bignum_sub(Bignum *res, const Bignum *a, const Bignum *b);
void bignum_mul(Bignum *res, const Bignum *a, const Bignum *b);
void bignum_div(Bignum *q, Bignum *r, const Bignum *a, const Bignum *b);
void bignum_mod(Bignum *r, const Bignum *a, const Bignum *mod);
void bignum_modpow(Bignum *res, const Bignum *base, const Bignum *exp, const Bignum *mod);

// RS2 Rev 225 RSA encryption wrapper
// Encrypts in_data of length in_len using Rev 225 keys, writes output to out_buf, returns byte length.
size_t rsa_encrypt(const uint8_t *in_data, size_t in_len, uint8_t *out_buf, size_t max_out);

#endif // RS2_RSA_H
