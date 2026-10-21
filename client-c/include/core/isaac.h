#ifndef RS2_ISAAC_H
#define RS2_ISAAC_H

#include <stdint.h>
#include <stddef.h>

#define ISAAC_SIZE 256

typedef struct {
    int32_t count;
    uint32_t rsl[ISAAC_SIZE];
    uint32_t mem[ISAAC_SIZE];
    uint32_t a;
    uint32_t b;
    uint32_t c;
} Isaac;

void isaac_init(Isaac *ctx, const uint32_t *seed, size_t seed_len);
uint32_t isaac_next(Isaac *ctx);

#endif // RS2_ISAAC_H
