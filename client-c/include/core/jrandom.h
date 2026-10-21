#ifndef RS2_JRANDOM_H
#define RS2_JRANDOM_H

#include <stdint.h>

// java.util.Random (48-bit LCG) and the single shared instance behind
// java.lang.Math.random(), which every Math.random() site in the original
// client draws from. Seeding it identically in Java and native makes those
// sites reproducible for oracles and replay.
typedef struct { uint64_t seed; } JRandom;

void jrandom_set_seed(JRandom *r, int64_t seed);
int32_t jrandom_next(JRandom *r, int bits);
double jrandom_next_double(JRandom *r);

// Math.random(): seeded from the clock on first use unless math_random_seed ran first.
void math_random_seed(int64_t seed);
double math_random(void);

#endif // RS2_JRANDOM_H
