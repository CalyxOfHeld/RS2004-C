#include "gfx/pixfont.h"
#include "gfx/pix2d.h"
#include "core/packet.h"
#include "core/jrandom.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int CHAR_LOOKUP[256];
static bool g_lookup_init = false;

void pixfont_init_lookup(void) {
    if (g_lookup_init) return;
    const char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!\"\xA3$%^&*()-_=+[{]};:'@#~,<.>/?\\| ";
    for (int i = 0; i < 256; i++) {
        CHAR_LOOKUP[i] = 74; // default character '='
    }
    for (int i = 0; charset[i] != '\0'; i++) {
        uint8_t c = (uint8_t)charset[i];
        CHAR_LOOKUP[c] = i;
    }
    g_lookup_init = true;
}

static uint32_t evaluate_tag(const char *tag) {
    if (strncmp(tag, "red", 3) == 0) return 0xFF0000;
    if (strncmp(tag, "gre", 3) == 0) return 0x00FF00;
    if (strncmp(tag, "blu", 3) == 0) return 0x0000FF;
    if (strncmp(tag, "yel", 3) == 0) return 0xFFFF00;
    if (strncmp(tag, "cya", 3) == 0) return 0x00FFFF;
    if (strncmp(tag, "mag", 3) == 0) return 0xFF00FF;
    if (strncmp(tag, "whi", 3) == 0) return 0xFFFFFF;
    if (strncmp(tag, "bla", 3) == 0) return 0x000000;
    if (strncmp(tag, "lre", 3) == 0) return 0xFF9040;
    if (strncmp(tag, "dre", 3) == 0) return 0x800000;
    if (strncmp(tag, "dbl", 3) == 0) return 0x000080;
    if (strncmp(tag, "or1", 3) == 0) return 0xFFB000;
    if (strncmp(tag, "or2", 3) == 0) return 0xFF7000;
    if (strncmp(tag, "or3", 3) == 0) return 0xFF3000;
    if (strncmp(tag, "gr1", 3) == 0) return 0xC0FF00;
    if (strncmp(tag, "gr2", 3) == 0) return 0x80FF00;
    if (strncmp(tag, "gr3", 3) == 0) return 0x40FF00;
    return 0;
}

bool pixfont_load(PixFont *font, Jagfile *title_jag, const char *name, Arena *arena) {
    if (!font || !title_jag || !name) return false;
    pixfont_init_lookup();
    memset(font, 0, sizeof(PixFont));

    char dat_name[32];
    snprintf(dat_name, sizeof(dat_name), "%s.dat", name);

    size_t dat_sz = 0;
    size_t idx_sz = 0;
    uint8_t *dat_buf = jagfile_read(title_jag, dat_name, NULL, &dat_sz, arena);
    uint8_t *idx_buf = jagfile_read(title_jag, "index.dat", NULL, &idx_sz, arena);

    if (!dat_buf || !idx_buf) return false;

    Packet dat_pkt = packet_wrap(dat_buf, dat_sz);
    Packet idx_pkt = packet_wrap(idx_buf, idx_sz);

    idx_pkt.pos = (size_t)(g2(&dat_pkt) + 4);
    int palette_count = g1(&idx_pkt);
    if (palette_count > 0) {
        idx_pkt.pos += (palette_count - 1) * 3;
    }

    for (int i = 0; i < 94; i++) {
        font->char_offset_x[i] = g1(&idx_pkt);
        font->char_offset_y[i] = g1(&idx_pkt);
        int w = font->char_mask_width[i] = g2(&idx_pkt);
        int h = font->char_mask_height[i] = g2(&idx_pkt);
        int order = g1(&idx_pkt);

        int total = w * h;
        font->char_mask[i] = (uint8_t*)arena_alloc(arena, total ? total : 1);

        if (order == 0) {
            for (int k = 0; k < total; k++) {
                font->char_mask[i][k] = (uint8_t)g1(&dat_pkt);
            }
        } else if (order == 1) {
            for (int x = 0; x < w; x++) {
                for (int y = 0; y < h; y++) {
                    font->char_mask[i][x + y * w] = (uint8_t)g1(&dat_pkt);
                }
            }
        }

        if (h > font->height) {
            font->height = h;
        }

        font->char_offset_x[i] = 1;
        font->char_advance[i] = w + 2;

        int left_sum = 0;
        for (int y = h / 7; y < h; y++) {
            left_sum += font->char_mask[i][y * w];
        }
        if (left_sum <= h / 7) {
            font->char_advance[i]--;
            font->char_offset_x[i] = 0;
        }

        int right_sum = 0;
        for (int y = h / 7; y < h; y++) {
            right_sum += font->char_mask[i][w - 1 + y * w];
        }
        if (right_sum <= h / 7) {
            font->char_advance[i]--;
        }
    }

    font->char_advance[94] = font->char_advance[8]; // space advance
    for (int i = 0; i < 256; i++) {
        font->draw_width[i] = font->char_advance[CHAR_LOOKUP[i]];
    }

    return true;
}

int pixfont_string_width(PixFont *font, const char *str) {
    if (!font || !str) return 0;
    int w = 0;
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '@' && i + 4 < len && str[i + 4] == '@') {
            i += 4;
        } else {
            w += font->draw_width[(uint8_t)str[i]];
        }
    }
    return w;
}

static void draw_char_mask(const uint8_t *mask, int x, int y, int w, int h, uint32_t color) {
    if (!mask || w <= 0 || h <= 0) return;
    int dst_off = x + y * pix2d_width;
    int dst_step = pix2d_width - w;
    int src_step = 0;
    int src_off = 0;
    int cut;

    if (y < pix2d_clip_top) {
        cut = pix2d_clip_top - y;
        h -= cut;
        y = pix2d_clip_top;
        src_off += cut * w;
        dst_off += cut * pix2d_width;
    }
    if (y + h >= pix2d_clip_bottom) {
        h -= (y + h - pix2d_clip_bottom + 1);
    }
    if (x < pix2d_clip_left) {
        cut = pix2d_clip_left - x;
        w -= cut;
        x = pix2d_clip_left;
        src_off += cut;
        dst_off += cut;
        src_step += cut;
        dst_step += cut;
    }
    if (x + w >= pix2d_clip_right) {
        cut = x + w - pix2d_clip_right + 1;
        w -= cut;
        src_step += cut;
        dst_step += cut;
    }

    if (w <= 0 || h <= 0) return;

    int w4 = -(w >> 2);
    int rem = -(w & 0x3);

    for (int row = -h; row < 0; row++) {
        for (int col = w4; col < 0; col++) {
            if (mask[src_off++] != 0) pix2d_pixels[dst_off++] = color; else dst_off++;
            if (mask[src_off++] != 0) pix2d_pixels[dst_off++] = color; else dst_off++;
            if (mask[src_off++] != 0) pix2d_pixels[dst_off++] = color; else dst_off++;
            if (mask[src_off++] != 0) pix2d_pixels[dst_off++] = color; else dst_off++;
        }
        for (int col = rem; col < 0; col++) {
            if (mask[src_off++] != 0) pix2d_pixels[dst_off++] = color; else dst_off++;
        }
        dst_off += dst_step;
        src_off += src_step;
    }
}

void pixfont_draw_string(PixFont *font, int x, int y, uint32_t color, const char *str) {
    if (!font || !str) return;
    int baseline = y - font->height;
    size_t len = strlen(str);

    for (size_t i = 0; i < len; i++) {
        int idx = CHAR_LOOKUP[(uint8_t)str[i]];
        if (idx >= 0 && idx < 94) {
            draw_char_mask(font->char_mask[idx],
                           x + font->char_offset_x[idx],
                           baseline + font->char_offset_y[idx],
                           font->char_mask_width[idx],
                           font->char_mask_height[idx],
                           color);
        }
        x += font->char_advance[idx];
    }
}

void pixfont_draw_string_center(PixFont *font, int x, int y, uint32_t color, const char *str) {
    if (!font || !str) return;
    int w = pixfont_string_width(font, str);
    pixfont_draw_string(font, x - w / 2, y, color, str);
}

void pixfont_draw_centered_wave225(PixFont *font,int x,int y,uint32_t color,const char *text,int cycle) {
    if(!font || !text)return;
    x-=pixfont_string_width(font,text)/2;y-=font->height;
    for(size_t i=0;text[i];i++) {
        int ch=CHAR_LOOKUP[(uint8_t)text[i]];
        if(ch!=94)draw_char_mask(font->char_mask[ch],x+font->char_offset_x[ch],
            y+font->char_offset_y[ch]+(int)(sin((double)i/2.0+(double)cycle/5.0)*5.0),
            font->char_mask_width[ch],font->char_mask_height[ch],color);
        x+=font->char_advance[ch];
    }
}

void pixfont_draw_string_right(PixFont *font, int x, int y, uint32_t color, const char *str) {
    if (!font || !str) return;
    int w = pixfont_string_width(font, str);
    pixfont_draw_string(font, x - w, y, color, str);
}

void pixfont_draw_string_taggable(PixFont *font, int x, int y, uint32_t default_color, const char *str, bool shadow) {
    if (!font || !str) return;
    int baseline = y - font->height;
    uint32_t cur_color = default_color;
    size_t len = strlen(str);

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '@' && i + 4 < len && str[i + 4] == '@') {
            cur_color = evaluate_tag(&str[i + 1]);
            i += 4;
        } else {
            int idx = CHAR_LOOKUP[(uint8_t)str[i]];
            if (idx >= 0 && idx < 94) {
                if (shadow) {
                    draw_char_mask(font->char_mask[idx],
                                   x + font->char_offset_x[idx] + 1,
                                   baseline + font->char_offset_y[idx] + 1,
                                   font->char_mask_width[idx],
                                   font->char_mask_height[idx],
                                   0x000000);
                }
                draw_char_mask(font->char_mask[idx],
                               x + font->char_offset_x[idx],
                               baseline + font->char_offset_y[idx],
                               font->char_mask_width[idx],
                               font->char_mask_height[idx],
                               cur_color);
            }
            x += font->char_advance[idx];
        }
    }
}

void pixfont_draw_string_taggable_center(PixFont *font, int x, int y, uint32_t default_color, const char *str, bool shadow) {
    if (!font || !str) return;
    int w = pixfont_string_width(font, str);
    pixfont_draw_string_taggable(font, x - w / 2, y, default_color, str, shadow);
}

static void draw_char_alpha225(const uint8_t *mask,int x,int y,int width,int height,uint32_t color,int alpha){
    if(!mask)return;
    uint32_t lit=(((color&0xff00ffu)*(uint32_t)alpha&0xff00ff00u)+((color&0xff00u)*(uint32_t)alpha&0xff0000u))>>8;
    uint32_t inverse=(uint32_t)(256-alpha);
    for(int row=0;row<height;row++)for(int col=0;col<width;col++){
        int px=x+col,py=y+row;
        if(!mask[row*width+col] || px<pix2d_clip_left || px>=pix2d_clip_right-1 || py<pix2d_clip_top || py>=pix2d_clip_bottom-1)continue;
        uint32_t *pixel=&pix2d_pixels[py*pix2d_width+px],old=*pixel;
        *pixel=lit+((((old&0xff00ffu)*inverse&0xff00ff00u)+((old&0xff00u)*inverse&0xff0000u))>>8);
    }
}

/* Original drawStringTooltip owns a java.util.Random, separate from Math. */
void pixfont_draw_tooltip225(PixFont *font,int x,int y,uint32_t color,const char *text,int seed){
    if(!font || !text)return;
    JRandom random;jrandom_set_seed(&random,seed);int alpha=(jrandom_next(&random,32)&31)+192;
    int baseline=y-font->height;size_t length=strlen(text);
    for(size_t i=0;i<length;i++){
        if(text[i]=='@' && i+4<length && text[i+4]=='@'){color=evaluate_tag(text+i+1);i+=4;continue;}
        int glyph=CHAR_LOOKUP[(uint8_t)text[i]];
        if(glyph<94){
            int px=x+font->char_offset_x[glyph],py=baseline+font->char_offset_y[glyph];
            draw_char_alpha225(font->char_mask[glyph],px+1,py+1,font->char_mask_width[glyph],font->char_mask_height[glyph],0,192);
            draw_char_alpha225(font->char_mask[glyph],px,py,font->char_mask_width[glyph],font->char_mask_height[glyph],color,alpha);
        }
        x+=font->char_advance[glyph];if((jrandom_next(&random,32)&3)==0)x++;
    }
}
