#include "core/isaac.h"
#include <string.h>

static void isaac_generate(Isaac *ctx) {
    ctx->b += ++ctx->c;
    for (int32_t i = 0; i < 256; i++) {
        uint32_t x = ctx->mem[i];
        switch (i & 0x3) {
            case 0: ctx->a ^= (ctx->a << 13); break;
            case 1: ctx->a ^= (ctx->a >> 6);  break;
            case 2: ctx->a ^= (ctx->a << 2);  break;
            case 3: ctx->a ^= (ctx->a >> 16); break;
        }
        ctx->a += ctx->mem[(i + 128) & 0xFF];
        uint32_t y;
        ctx->mem[i] = y = ctx->mem[(x >> 2) & 0xFF] + ctx->a + ctx->b;
        ctx->rsl[i] = ctx->b = ctx->mem[(y >> 10) & 0xFF] + x;
    }
}

#define MIX(a, b, c, d, e, f, g, h) \
    a ^= b << 11; d += a; b += c; \
    b ^= c >> 2;  e += b; c += d; \
    c ^= d << 8;  f += c; d += e; \
    d ^= e >> 16; g += d; e += f; \
    e ^= f << 10; h += e; f += g; \
    f ^= g >> 4;  a += f; g += h; \
    g ^= h << 8;  b += g; h += a; \
    h ^= a >> 9;  c += h; a += b

void isaac_init(Isaac *ctx, const uint32_t *seed, size_t seed_len) {
    memset(ctx, 0, sizeof(Isaac));

    for (size_t i = 0; i < seed_len && i < 256; i++) {
        ctx->rsl[i] = seed[i];
    }

    uint32_t a = 0x9e3779b9;
    uint32_t b = 0x9e3779b9;
    uint32_t c = 0x9e3779b9;
    uint32_t d = 0x9e3779b9;
    uint32_t e = 0x9e3779b9;
    uint32_t f = 0x9e3779b9;
    uint32_t g = 0x9e3779b9;
    uint32_t h = 0x9e3779b9;

    for (int32_t i = 0; i < 4; i++) {
        MIX(a, b, c, d, e, f, g, h);
    }

    for (int32_t i = 0; i < 256; i += 8) {
        a += ctx->rsl[i];
        b += ctx->rsl[i + 1];
        c += ctx->rsl[i + 2];
        d += ctx->rsl[i + 3];
        e += ctx->rsl[i + 4];
        f += ctx->rsl[i + 5];
        g += ctx->rsl[i + 6];
        h += ctx->rsl[i + 7];
        MIX(a, b, c, d, e, f, g, h);
        ctx->mem[i]     = a;
        ctx->mem[i + 1] = b;
        ctx->mem[i + 2] = c;
        ctx->mem[i + 3] = d;
        ctx->mem[i + 4] = e;
        ctx->mem[i + 5] = f;
        ctx->mem[i + 6] = g;
        ctx->mem[i + 7] = h;
    }

    for (int32_t i = 0; i < 256; i += 8) {
        a += ctx->mem[i];
        b += ctx->mem[i + 1];
        c += ctx->mem[i + 2];
        d += ctx->mem[i + 3];
        e += ctx->mem[i + 4];
        f += ctx->mem[i + 5];
        g += ctx->mem[i + 6];
        h += ctx->mem[i + 7];
        MIX(a, b, c, d, e, f, g, h);
        ctx->mem[i]     = a;
        ctx->mem[i + 1] = b;
        ctx->mem[i + 2] = c;
        ctx->mem[i + 3] = d;
        ctx->mem[i + 4] = e;
        ctx->mem[i + 5] = f;
        ctx->mem[i + 6] = g;
        ctx->mem[i + 7] = h;
    }

    isaac_generate(ctx);
    ctx->count = 256;
}

uint32_t isaac_next(Isaac *ctx) {
    if (ctx->count-- == 0) {
        isaac_generate(ctx);
        ctx->count = 255;
    }
    return ctx->rsl[ctx->count];
}
