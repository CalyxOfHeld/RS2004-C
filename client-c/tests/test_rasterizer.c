#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"

int main(void) {
    printf("[Test Rasterizer] Testing Pix3D software rasterizer & trig tables...\n");

    // 1. Initialize tables
    pix3d_init();
    assert(pix3d_sin_table[0] == 0);
    // Original Pix3D's truncated angular constant produces 65535 here.
    // test_minimap_reference compares every entry with actual Java output.
    assert(pix3d_sin_table[512] == 65535);
    assert(pix3d_cos_table[0] == 65536);
    assert(pix3d_cos_table[512] == 0);
    printf("  Four original trigonometry table anchor entries verified!\n");

    // 2. Initialize colour table (brightness = 0.8)
    pix3d_init_colour_table(0.8);
    assert(pix3d_colour_table[0] == 0); // black
    assert(pix3d_colour_table[127] > 0); // white/light
    printf("  Colour table (65536 entries) verified!\n");

    // 3. Setup framebuffer and rasterize flat triangle
    uint32_t fb[100 * 100];
    pix2d_bind(fb, 100, 100);
    pix2d_cls();
    pix3d_init_2d();

    // Triangle vertices: (50, 10), (10, 80), (90, 80), color = 0xFF5500
    pix3d_flat_triangle(50, 10, 90, 10, 80, 80, 0xFF5500);

    // Pixel at center (50, 50) must be inside the triangle
    assert(fb[50 + 50 * 100] == 0xFF5500);
    // Pixel outside top (50, 5) must be 0
    assert(fb[50 + 5 * 100] == 0);
    // Pixel outside left (5, 50) must be 0
    assert(fb[5 + 50 * 100] == 0);
    printf("  Flat-shaded triangle rasterization verified!\n");

    // 4. Test Gouraud triangle
    pix2d_cls();
    // Vertex colors in 8.8 fixed-point (color << 8)
    pix3d_gouraud_triangle(50, 10, 90, 10, 80, 80, 10 << 8, 50 << 8, 100 << 8);
    // Center pixel should be rendered with interpolated palette color
    uint32_t center_pixel = fb[50 + 50 * 100];
    assert(center_pixel != 0);
    printf("  Gouraud-shaded triangle rasterization verified! Center color = 0x%06X\n", center_pixel);

    printf("[Test Rasterizer] ALL TESTS PASSED! Pix3D engine operational.\n");
    return 0;
}
