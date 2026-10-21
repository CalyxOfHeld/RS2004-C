#include <stdio.h>
#include <assert.h>
#include "core/isaac.h"

int main(void) {
    printf("[Test Isaac] Running ISAAC PRNG parity test...\n");

    uint32_t seed[4] = { 1, 2, 3, 4 };
    Isaac isaac;
    isaac_init(&isaac, seed, 4);

    int32_t expected[5] = {
        -621246914,
        1957022519,
        -1345000077,
        -2021884860,
        -1882702437
    };

    for (int i = 0; i < 5; i++) {
        int32_t val = (int32_t)isaac_next(&isaac);
        printf("  Value %d: got %d, expected %d\n", i, val, expected[i]);
        assert(val == expected[i]);
    }

    printf("[Test Isaac] PASSED! Bit-for-bit parity with 2004scape reference.\n");
    return 0;
}
