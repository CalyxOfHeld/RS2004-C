#include "core/jrandom.h"
#include <stdbool.h>
#include <time.h>

#define JRANDOM_MULTIPLIER 0x5DEECE66DULL
#define JRANDOM_ADDEND 0xBULL
#define JRANDOM_MASK ((1ULL << 48) - 1)

void jrandom_set_seed(JRandom *r, int64_t seed) {
    r->seed = ((uint64_t)seed ^ JRANDOM_MULTIPLIER) & JRANDOM_MASK;
}

int32_t jrandom_next(JRandom *r, int bits) {
    r->seed = (r->seed * JRANDOM_MULTIPLIER + JRANDOM_ADDEND) & JRANDOM_MASK;
    return (int32_t)(uint32_t)(r->seed >> (48 - bits));
}

double jrandom_next_double(JRandom *r) {
    int64_t hi = (int64_t)jrandom_next(r, 26);
    int64_t lo = (int64_t)jrandom_next(r, 27);
    return (double)((hi << 27) + lo) * (1.0 / (double)(1ULL << 53));
}

static JRandom math_rng;
static bool math_rng_ready;

void math_random_seed(int64_t seed) {
    jrandom_set_seed(&math_rng, seed);
    math_rng_ready = true;
}

double math_random(void) {
    if (!math_rng_ready) {
        // Java seeds Math's generator from System.nanoTime(); any clock value is equivalent.
        math_random_seed((int64_t)time(NULL) * 1000003LL + (int64_t)clock());
    }
    return jrandom_next_double(&math_rng);
}
