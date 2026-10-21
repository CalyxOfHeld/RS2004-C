#include "net/rsa.h"
#include <string.h>
#include <stdio.h>

static const char *RSA_MOD_HEX = "88c38748a58228f7261cdc340b5691d7d0975dee0ecdb717609e6bf971eb3fe723ef9d130e4686813739768ad9472eb46d8bfcc042c1a5fcb05e931f632eea5d";
static const char *RSA_EXP_HEX = "81f390b2cf8ca7039ee507975951d5a0b15a87bf8b3f99c966834118c50fd94d";

static void bignum_normalize(Bignum *n) {
    while (n->length > 0 && n->limbs[n->length - 1] == 0) {
        n->length--;
    }
}

void bignum_zero(Bignum *n) {
    memset(n->limbs, 0, sizeof(n->limbs));
    n->length = 0;
}

void bignum_copy(Bignum *dst, const Bignum *src) {
    memcpy(dst->limbs, src->limbs, sizeof(dst->limbs));
    dst->length = src->length;
}

void bignum_from_bytes(Bignum *n, const uint8_t *bytes, size_t len) {
    bignum_zero(n);
    if (len == 0) return;

    for (size_t i = 0; i < len; i++) {
        size_t byte_idx = len - 1 - i;
        size_t limb_idx = i / 4;
        size_t shift = (i % 4) * 8;
        if (limb_idx < BIGNUM_MAX_LIMBS) {
            n->limbs[limb_idx] |= ((uint32_t)bytes[byte_idx]) << shift;
            if ((int)limb_idx >= n->length) {
                n->length = (int)limb_idx + 1;
            }
        }
    }
    bignum_normalize(n);
}

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

void bignum_from_hex(Bignum *n, const char *hex) {
    bignum_zero(n);
    size_t len = strlen(hex);
    if (len == 0) return;

    uint8_t bytes[256];
    size_t byte_len = (len + 1) / 2;
    memset(bytes, 0, sizeof(bytes));

    for (size_t i = 0; i < len; i++) {
        size_t byte_idx = byte_len - 1 - (i / 2);
        int nibble = hex_val(hex[len - 1 - i]);
        if (i % 2 == 0) {
            bytes[byte_idx] |= (uint8_t)nibble;
        } else {
            bytes[byte_idx] |= (uint8_t)(nibble << 4);
        }
    }

    bignum_from_bytes(n, bytes, byte_len);
}

size_t bignum_to_java_bytes(const Bignum *n, uint8_t *out_buf, size_t max_out) {
    if (n->length == 0) {
        if (max_out > 0) out_buf[0] = 0;
        return 1;
    }

    // Determine the highest non-zero byte
    uint32_t top = n->limbs[n->length - 1];
    int top_bytes = 4;
    if ((top & 0xFF000000) == 0) {
        top_bytes = 3;
        if ((top & 0x00FF0000) == 0) {
            top_bytes = 2;
            if ((top & 0x0000FF00) == 0) {
                top_bytes = 1;
            }
        }
    }

    size_t num_bytes = (n->length - 1) * 4 + top_bytes;

    // Check if MSB of highest byte is set (requires extra 0x00 prefix in Java BigInteger)
    uint8_t highest_byte = (uint8_t)(top >> ((top_bytes - 1) * 8));
    bool prepend_zero = (highest_byte & 0x80) != 0;

    size_t total_len = num_bytes + (prepend_zero ? 1 : 0);
    if (total_len > max_out) {
        total_len = max_out;
    }

    size_t out_pos = 0;
    if (prepend_zero && out_pos < max_out) {
        out_buf[out_pos++] = 0x00;
    }

    for (int i = (int)num_bytes - 1; i >= 0 && out_pos < max_out; i--) {
        int limb_idx = i / 4;
        int shift = (i % 4) * 8;
        out_buf[out_pos++] = (uint8_t)((n->limbs[limb_idx] >> shift) & 0xFF);
    }

    return total_len;
}

int bignum_cmp(const Bignum *a, const Bignum *b) {
    if (a->length != b->length) {
        return a->length > b->length ? 1 : -1;
    }
    for (int i = a->length - 1; i >= 0; i--) {
        if (a->limbs[i] != b->limbs[i]) {
            return a->limbs[i] > b->limbs[i] ? 1 : -1;
        }
    }
    return 0;
}

void bignum_add(Bignum *res, const Bignum *a, const Bignum *b) {
    Bignum tmp;
    bignum_zero(&tmp);

    int max_len = a->length > b->length ? a->length : b->length;
    uint64_t carry = 0;

    for (int i = 0; i < max_len || carry; i++) {
        if (i >= BIGNUM_MAX_LIMBS) break;
        uint64_t sum = carry;
        if (i < a->length) sum += a->limbs[i];
        if (i < b->length) sum += b->limbs[i];
        tmp.limbs[i] = (uint32_t)sum;
        carry = sum >> 32;
        tmp.length = i + 1;
    }

    bignum_normalize(&tmp);
    bignum_copy(res, &tmp);
}

void bignum_sub(Bignum *res, const Bignum *a, const Bignum *b) {
    Bignum tmp;
    bignum_zero(&tmp);

    int64_t borrow = 0;
    for (int i = 0; i < a->length; i++) {
        int64_t diff = (int64_t)a->limbs[i] - borrow;
        if (i < b->length) diff -= (int64_t)b->limbs[i];
        if (diff < 0) {
            diff += ((int64_t)1 << 32);
            borrow = 1;
        } else {
            borrow = 0;
        }
        tmp.limbs[i] = (uint32_t)diff;
    }
    tmp.length = a->length;
    bignum_normalize(&tmp);
    bignum_copy(res, &tmp);
}

void bignum_mul(Bignum *res, const Bignum *a, const Bignum *b) {
    Bignum tmp;
    bignum_zero(&tmp);

    if (a->length == 0 || b->length == 0) {
        bignum_zero(res);
        return;
    }

    for (int i = 0; i < a->length; i++) {
        uint64_t carry = 0;
        for (int j = 0; j < b->length || carry; j++) {
            int k = i + j;
            if (k >= BIGNUM_MAX_LIMBS) break;
            uint64_t cur = (uint64_t)tmp.limbs[k] + carry;
            if (j < b->length) {
                cur += (uint64_t)a->limbs[i] * (uint64_t)b->limbs[j];
            }
            tmp.limbs[k] = (uint32_t)cur;
            carry = cur >> 32;
            if (k >= tmp.length) {
                tmp.length = k + 1;
            }
        }
    }

    bignum_normalize(&tmp);
    bignum_copy(res, &tmp);
}

static void bignum_shift_left_bits(Bignum *res, const Bignum *a, int bits) {
    bignum_zero(res);
    if (a->length == 0) return;

    int limb_shift = bits / 32;
    int bit_shift = bits % 32;

    uint32_t carry = 0;
    for (int i = 0; i < a->length; i++) {
        int dst = i + limb_shift;
        if (dst >= BIGNUM_MAX_LIMBS) break;
        uint64_t val = ((uint64_t)a->limbs[i] << bit_shift) | carry;
        res->limbs[dst] = (uint32_t)val;
        carry = (uint32_t)(val >> 32);
        res->length = dst + 1;
    }
    if (carry && (a->length + limb_shift < BIGNUM_MAX_LIMBS)) {
        res->limbs[a->length + limb_shift] = carry;
        res->length = a->length + limb_shift + 1;
    }
    bignum_normalize(res);
}

static int bignum_bit_length(const Bignum *n) {
    if (n->length == 0) return 0;
    int bits = (n->length - 1) * 32;
    uint32_t top = n->limbs[n->length - 1];
    while (top > 0) {
        bits++;
        top >>= 1;
    }
    return bits;
}

void bignum_div(Bignum *q, Bignum *r, const Bignum *a, const Bignum *b) {
    if (b->length == 0) {
        // Division by zero
        if (q) bignum_zero(q);
        if (r) bignum_zero(r);
        return;
    }

    if (bignum_cmp(a, b) < 0) {
        if (q) bignum_zero(q);
        if (r) bignum_copy(r, a);
        return;
    }

    Bignum rem;
    bignum_copy(&rem, a);

    Bignum quot;
    bignum_zero(&quot);

    int a_bits = bignum_bit_length(a);
    int b_bits = bignum_bit_length(b);
    int shift = a_bits - b_bits;

    Bignum shifted_b;
    while (shift >= 0) {
        bignum_shift_left_bits(&shifted_b, b, shift);
        if (bignum_cmp(&rem, &shifted_b) >= 0) {
            bignum_sub(&rem, &rem, &shifted_b);
            int limb_idx = shift / 32;
            int bit_idx = shift % 32;
            quot.limbs[limb_idx] |= ((uint32_t)1 << bit_idx);
            if (limb_idx >= quot.length) {
                quot.length = limb_idx + 1;
            }
        }
        shift--;
    }

    bignum_normalize(&quot);
    bignum_normalize(&rem);

    if (q) bignum_copy(q, &quot);
    if (r) bignum_copy(r, &rem);
}

void bignum_mod(Bignum *r, const Bignum *a, const Bignum *mod) {
    bignum_div(NULL, r, a, mod);
}

void bignum_modpow(Bignum *res, const Bignum *base, const Bignum *exp, const Bignum *mod) {
    Bignum r;
    bignum_zero(&r);
    r.limbs[0] = 1;
    r.length = 1;

    Bignum b;
    bignum_mod(&b, base, mod);

    int exp_bits = bignum_bit_length(exp);

    for (int i = 0; i < exp_bits; i++) {
        int limb_idx = i / 32;
        int bit_idx = i % 32;
        if ((exp->limbs[limb_idx] >> bit_idx) & 1) {
            Bignum tmp;
            bignum_mul(&tmp, &r, &b);
            bignum_mod(&r, &tmp, mod);
        }

        if (i < exp_bits - 1) {
            Bignum tmp;
            bignum_mul(&tmp, &b, &b);
            bignum_mod(&b, &tmp, mod);
        }
    }

    bignum_copy(res, &r);
}

size_t rsa_encrypt(const uint8_t *in_data, size_t in_len, uint8_t *out_buf, size_t max_out) {
    Bignum m, exp, mod, cipher;

    bignum_from_bytes(&m, in_data, in_len);
    bignum_from_hex(&exp, RSA_EXP_HEX);
    bignum_from_hex(&mod, RSA_MOD_HEX);

    bignum_modpow(&cipher, &m, &exp, &mod);

    return bignum_to_java_bytes(&cipher, out_buf, max_out);
}
