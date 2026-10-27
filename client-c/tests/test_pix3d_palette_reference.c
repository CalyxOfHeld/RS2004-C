#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/pix8.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int read_i32(FILE *f, int32_t *v) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) return 0;
    *v = (int32_t)((uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3]);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "usage: test_pix3d_palette_reference textures-archive fixture\n"); return 2; }
    FILE *fixture = fopen(argv[2], "rb");
    if (!fixture) { perror(argv[2]); return 2; }
    int32_t magic;
    if (!read_i32(fixture, &magic) || magic != 0x50335031) { fprintf(stderr, "bad P3P1 fixture\n"); return 2; }
    unsigned char dbits[8];
    if (fread(dbits, 1, 8, fixture) != 8) return 2;
    uint64_t bits = 0; for (int i = 0; i < 8; i++) bits = (bits << 8) | dbits[i];
    union { uint64_t u; double d; } brightness = { bits };
    pix3d_init_colour_table(brightness.d);
    int table_differences = 0, dimension_differences = 0, palette_differences = 0;
    int pixel_differences = 0, average_differences = 0;
    for (int i = 0; i < 65536; i++) {
        int32_t expected;
        if (!read_i32(fixture, &expected)) return 2;
        if ((uint32_t)expected != (uint32_t)pix3d_colour_table[i]) table_differences++;
    }
    Jagfile *archive = jagfile_load_file(argv[1], NULL);
    if (!archive) { fprintf(stderr, "cannot load textures archive\n"); return 2; }
    int32_t texture_count;
    if (!read_i32(fixture, &texture_count)) return 2;
    int seen = 0;
    for (int n = 0; n < texture_count; n++) {
        int32_t id, width, height, palette_count, pixel_count, expected_average;
        if (!read_i32(fixture,&id)||!read_i32(fixture,&width)||!read_i32(fixture,&height)||!read_i32(fixture,&palette_count)) return 2;
        char name[8]; snprintf(name,sizeof(name),"%d",id);
        Pix8 *image = pix8_load(archive,name,0,NULL);
        if (!image) { fprintf(stderr,"missing decoded texture %d\n",id); return 1; }
        if (pix3d_low_detail && image->crop_w == 128) pix8_shrink(image,NULL);
        else pix8_crop(image,NULL);
        if (image->width != width || image->height != height || image->palette_count != palette_count) dimension_differences++;
        uint32_t *palette = malloc((size_t)palette_count * sizeof(uint32_t));
        if (!palette) return 2;
        for (int i=0;i<palette_count;i++) {
            int32_t value; if(!read_i32(fixture,&value)) return 2;
            palette[i]=(uint32_t)value;
            if(i>=image->palette_count || image->palette[i]!=palette[i]) palette_differences++;
        }
        if(!read_i32(fixture,&pixel_count)) return 2;
        if(pixel_count != image->width*image->height) pixel_differences++;
        for(int i=0;i<pixel_count;i++) {
            int expected=fgetc(fixture); if(expected==EOF) return 2;
            if(i>=image->width*image->height || image->pixels[i]!=(unsigned char)expected) pixel_differences++;
        }
        if(!read_i32(fixture,&expected_average)) return 2;
        pix3d_set_texture_palette_gamma(id,palette,palette_count,brightness.d);
        if(pix3d_texture_average(id)!=(int32_t)expected_average) average_differences++;
        free(palette); pix8_free(image); seen++;
    }
    fclose(fixture);
    int differences = table_differences + dimension_differences + palette_differences + pixel_differences + average_differences;
    printf("Palette/texture oracle: %d textures, %d HSL colors; table=%d dimensions=%d palette=%d pixels=%d average=%d mismatches\n",
        seen,65536,table_differences,dimension_differences,palette_differences,pixel_differences,average_differences);
    return differences ? 1 : 0;
}
