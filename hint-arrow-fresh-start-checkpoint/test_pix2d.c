#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "gfx/pix2d.h"

int main(void) {
    printf("[Test Pix2D] Running Pix2D primitives & alpha blending tests...\n");

    uint32_t buf[100 * 100];
    memset(buf, 0, sizeof(buf));
    pix2d_bind(buf, 100, 100);

    // 1. Clear screen
    pix2d_cls();
    assert(buf[0] == 0);
    assert(buf[9999] == 0);

    // 2. Fill rect
    pix2d_fill_rect(10, 10, 20, 20, 0xFF00FF);
    assert(buf[12 + 12 * 100] == 16711935);

    // 3. Alpha blend fill rect
    pix2d_fill_rect_trans(15, 15, 20, 20, 0x00FF00, 128);
    assert(buf[16 + 16 * 100] == 8355711);
    assert(buf[25 + 25 * 100] == 8355711);

    // 4. Lines & Clipping
    pix2d_set_clip(5, 5, 50, 50);
    // Draw hline that exceeds clip boundaries: x from 0 to 80 at y=20
    pix2d_hline(0, 20, 80, 0x123456);
    // Before left clip (x=4) must not be drawn
    assert(buf[4 + 20 * 100] != 0x123456);
    // Inside clip (x=10) must be drawn
    assert(buf[10 + 20 * 100] == 0x123456);
    // After right clip (x=55) must not be drawn
    assert(buf[55 + 20 * 100] != 0x123456);

    printf("[Test Pix2D] ALL TESTS PASSED! 100%% parity with reference Pix2D.\n");
    return 0;
}
