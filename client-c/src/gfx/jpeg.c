#include "gfx/jpeg.h"
#include <stdlib.h>
#include <string.h>

// Integer arithmetic below follows IJG libjpeg 6b, which the original client
// reaches through java.awt.Toolkit: jidctint.c (islow), jdsample.c (fancy
// h2v1/h2v2), jdcolor.c (ycc_rgb_convert) and jdmaster.c range limiting.

#define JPEG_MAX_COMPONENTS 4

static const uint8_t jpeg_natural_order[64] = {
     0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

typedef struct {
    bool     present;
    int32_t  maxcode[18];
    int32_t  valptr[17];
    int32_t  mincode[17];
    uint8_t  values[256];
} JpegHuffman;

typedef struct {
    int      id, h, v, tq, td, ta;
    int      blocks_w, blocks_h;   // allocated block grid
    int      ds_w, ds_h;           // downsampled_width/height
    int      dc_pred;
    uint8_t *plane;                // blocks_w*8 x blocks_h*8 samples
} JpegComponent;

typedef struct {
    const uint8_t *data;
    size_t   length, pos;
    uint32_t bits;
    int      bit_count;
    bool     marker_hit;
    int32_t  quant[4][64];         // natural order
    bool     quant_present[4];
    JpegHuffman dc[4], ac[4];
    JpegComponent comp[JPEG_MAX_COMPONENTS];
    int      ncomp, width, height, hmax, vmax, mcux, mcuy;
    int      restart_interval;
    uint8_t  range_base[5 * 256 + 128];
} JpegDecoder;

static int jpeg_u16(JpegDecoder *d) {
    if (d->pos + 2 > d->length) return -1;
    int v = (d->data[d->pos] << 8) | d->data[d->pos + 1];
    d->pos += 2;
    return v;
}

// jdmaster.c prepare_range_limit_table, MAXJSAMPLE=255, CENTERJSAMPLE=128.
static void jpeg_prepare_range_limit(JpegDecoder *d) {
    uint8_t *table = d->range_base + 256;
    memset(d->range_base, 0, 256);
    for (int i = 0; i <= 255; i++) table[i] = (uint8_t)i;
    table += 128;
    for (int i = 128; i < 512; i++) table[i] = 255;
    memset(table + 512, 0, 384);
    memcpy(table + 1024 - 128, d->range_base + 256, 128);
}

static bool jpeg_build_huffman(JpegHuffman *table, const uint8_t counts[16], const uint8_t *values, int total) {
    int code = 0, k = 0;
    memset(table, 0, sizeof(*table));
    for (int len = 1; len <= 16; len++) {
        table->valptr[len] = k;
        table->mincode[len] = code;
        code += counts[len - 1];
        k += counts[len - 1];
        table->maxcode[len] = counts[len - 1] ? code - 1 : -1;
        if (code > (1 << len)) return false;
        code <<= 1;
    }
    table->maxcode[17] = 0x7FFFFFFF;
    memcpy(table->values, values, (size_t)total);
    table->present = true;
    return true;
}

static void jpeg_fill_bits(JpegDecoder *d) {
    while (d->bit_count <= 24) {
        int byte = 0;
        if (!d->marker_hit && d->pos < d->length) {
            byte = d->data[d->pos];
            if (byte == 0xFF) {
                int next = d->pos + 1 < d->length ? d->data[d->pos + 1] : 0;
                if (next == 0x00) d->pos += 2;
                else { d->marker_hit = true; byte = 0; }
            } else {
                d->pos++;
            }
        }
        // Like libjpeg, missing data after a marker is filled with zeros.
        d->bits |= (uint32_t)byte << (24 - d->bit_count);
        d->bit_count += 8;
    }
}

static int jpeg_get_bits(JpegDecoder *d, int n) {
    if (n == 0) return 0;
    jpeg_fill_bits(d);
    int v = (int)(d->bits >> (32 - n));
    d->bits <<= n;
    d->bit_count -= n;
    return v;
}

static int jpeg_decode_huffman(JpegDecoder *d, const JpegHuffman *table) {
    int code = 0;
    for (int len = 1; len <= 16; len++) {
        code = (code << 1) | jpeg_get_bits(d, 1);
        if (table->maxcode[len] >= 0 && code <= table->maxcode[len])
            return table->values[table->valptr[len] + code - table->mincode[len]];
    }
    return -1;
}

static int jpeg_extend(int v, int s) {
    return v < (1 << (s - 1)) ? v - (1 << s) + 1 : v;
}

#define CONST_BITS 13
#define PASS1_BITS 2
#define FIX_0_298631336 ((int32_t)2446)
#define FIX_0_390180644 ((int32_t)3196)
#define FIX_0_541196100 ((int32_t)4433)
#define FIX_0_765366865 ((int32_t)6270)
#define FIX_0_899976223 ((int32_t)7373)
#define FIX_1_175875602 ((int32_t)9633)
#define FIX_1_501321110 ((int32_t)12299)
#define FIX_1_847759065 ((int32_t)15137)
#define FIX_1_961570560 ((int32_t)16069)
#define FIX_2_053119869 ((int32_t)16819)
#define FIX_2_562915447 ((int32_t)20995)
#define FIX_3_072711026 ((int32_t)25172)
#define DESCALE(x, n) (((x) + ((int32_t)1 << ((n) - 1))) >> (n))
#define RANGE_MASK 1023

// jidctint.c jpeg_idct_islow; coef is natural order, quant applied here.
static void jpeg_idct_islow(const int16_t *coef, const int32_t *quant, const uint8_t *range_limit,
                            uint8_t *out, int stride) {
    int32_t ws[64];
    for (int c = 0; c < 8; c++) {
        const int16_t *in = coef + c;
        const int32_t *q = quant + c;
        int32_t *w = ws + c;
        if (!in[8] && !in[16] && !in[24] && !in[32] && !in[40] && !in[48] && !in[56]) {
            int32_t dc = (in[0] * q[0]) << PASS1_BITS;
            for (int r = 0; r < 8; r++) w[r * 8] = dc;
            continue;
        }
        int32_t z1, z2, z3, z4, z5, tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
        z2 = in[16] * q[16];
        z3 = in[48] * q[48];
        z1 = (z2 + z3) * FIX_0_541196100;
        tmp2 = z1 + z3 * (-FIX_1_847759065);
        tmp3 = z1 + z2 * FIX_0_765366865;
        z2 = in[0] * q[0];
        z3 = in[32] * q[32];
        tmp0 = (z2 + z3) << CONST_BITS;
        tmp1 = (z2 - z3) << CONST_BITS;
        tmp10 = tmp0 + tmp3; tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2; tmp12 = tmp1 - tmp2;
        tmp0 = in[56] * q[56];
        tmp1 = in[40] * q[40];
        tmp2 = in[24] * q[24];
        tmp3 = in[8] * q[8];
        z1 = tmp0 + tmp3; z2 = tmp1 + tmp2; z3 = tmp0 + tmp2; z4 = tmp1 + tmp3;
        z5 = (z3 + z4) * FIX_1_175875602;
        tmp0 *= FIX_0_298631336; tmp1 *= FIX_2_053119869;
        tmp2 *= FIX_3_072711026; tmp3 *= FIX_1_501321110;
        z1 *= -FIX_0_899976223; z2 *= -FIX_2_562915447;
        z3 *= -FIX_1_961570560; z4 *= -FIX_0_390180644;
        z3 += z5; z4 += z5;
        tmp0 += z1 + z3; tmp1 += z2 + z4; tmp2 += z2 + z3; tmp3 += z1 + z4;
        w[0]  = DESCALE(tmp10 + tmp3, CONST_BITS - PASS1_BITS);
        w[56] = DESCALE(tmp10 - tmp3, CONST_BITS - PASS1_BITS);
        w[8]  = DESCALE(tmp11 + tmp2, CONST_BITS - PASS1_BITS);
        w[48] = DESCALE(tmp11 - tmp2, CONST_BITS - PASS1_BITS);
        w[16] = DESCALE(tmp12 + tmp1, CONST_BITS - PASS1_BITS);
        w[40] = DESCALE(tmp12 - tmp1, CONST_BITS - PASS1_BITS);
        w[24] = DESCALE(tmp13 + tmp0, CONST_BITS - PASS1_BITS);
        w[32] = DESCALE(tmp13 - tmp0, CONST_BITS - PASS1_BITS);
    }
    for (int r = 0; r < 8; r++) {
        const int32_t *w = ws + r * 8;
        uint8_t *o = out + r * stride;
        if (!w[1] && !w[2] && !w[3] && !w[4] && !w[5] && !w[6] && !w[7]) {
            uint8_t v = range_limit[DESCALE(w[0], PASS1_BITS + 3) & RANGE_MASK];
            for (int i = 0; i < 8; i++) o[i] = v;
            continue;
        }
        int32_t z1, z2, z3, z4, z5, tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
        z2 = w[2]; z3 = w[6];
        z1 = (z2 + z3) * FIX_0_541196100;
        tmp2 = z1 + z3 * (-FIX_1_847759065);
        tmp3 = z1 + z2 * FIX_0_765366865;
        tmp0 = (w[0] + w[4]) << CONST_BITS;
        tmp1 = (w[0] - w[4]) << CONST_BITS;
        tmp10 = tmp0 + tmp3; tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2; tmp12 = tmp1 - tmp2;
        tmp0 = w[7]; tmp1 = w[5]; tmp2 = w[3]; tmp3 = w[1];
        z1 = tmp0 + tmp3; z2 = tmp1 + tmp2; z3 = tmp0 + tmp2; z4 = tmp1 + tmp3;
        z5 = (z3 + z4) * FIX_1_175875602;
        tmp0 *= FIX_0_298631336; tmp1 *= FIX_2_053119869;
        tmp2 *= FIX_3_072711026; tmp3 *= FIX_1_501321110;
        z1 *= -FIX_0_899976223; z2 *= -FIX_2_562915447;
        z3 *= -FIX_1_961570560; z4 *= -FIX_0_390180644;
        z3 += z5; z4 += z5;
        tmp0 += z1 + z3; tmp1 += z2 + z4; tmp2 += z2 + z3; tmp3 += z1 + z4;
        const int shift = CONST_BITS + PASS1_BITS + 3;
        o[0] = range_limit[DESCALE(tmp10 + tmp3, shift) & RANGE_MASK];
        o[7] = range_limit[DESCALE(tmp10 - tmp3, shift) & RANGE_MASK];
        o[1] = range_limit[DESCALE(tmp11 + tmp2, shift) & RANGE_MASK];
        o[6] = range_limit[DESCALE(tmp11 - tmp2, shift) & RANGE_MASK];
        o[2] = range_limit[DESCALE(tmp12 + tmp1, shift) & RANGE_MASK];
        o[5] = range_limit[DESCALE(tmp12 - tmp1, shift) & RANGE_MASK];
        o[3] = range_limit[DESCALE(tmp13 + tmp0, shift) & RANGE_MASK];
        o[4] = range_limit[DESCALE(tmp13 - tmp0, shift) & RANGE_MASK];
    }
}

static bool jpeg_decode_block(JpegDecoder *d, JpegComponent *c, int bx, int by) {
    int16_t coef[64];
    memset(coef, 0, sizeof(coef));
    int t = jpeg_decode_huffman(d, &d->dc[c->td]);
    if (t < 0 || t > 15) return false;
    int diff = t ? jpeg_extend(jpeg_get_bits(d, t), t) : 0;
    c->dc_pred += diff;
    coef[0] = (int16_t)c->dc_pred;
    for (int k = 1; k < 64; k++) {
        int rs = jpeg_decode_huffman(d, &d->ac[c->ta]);
        if (rs < 0) return false;
        int r = rs >> 4, s = rs & 15;
        if (s) {
            k += r;
            if (k > 63) break;
            coef[jpeg_natural_order[k]] = (int16_t)jpeg_extend(jpeg_get_bits(d, s), s);
        } else {
            if (r != 15) break;
            k += 15;
        }
    }
    if (bx >= c->blocks_w || by >= c->blocks_h) return true;
    int stride = c->blocks_w * 8;
    jpeg_idct_islow(coef, d->quant[c->tq], d->range_base + 256 + 128,
                    c->plane + (size_t)by * 8 * stride + (size_t)bx * 8, stride);
    return true;
}

static bool jpeg_restart(JpegDecoder *d) {
    d->bits = 0; d->bit_count = 0;
    if (d->marker_hit || (d->pos + 1 < d->length && d->data[d->pos] == 0xFF)) {
        while (d->pos < d->length && d->data[d->pos] == 0xFF) d->pos++;
        if (d->pos < d->length && d->data[d->pos] >= 0xD0 && d->data[d->pos] <= 0xD7) d->pos++;
        else return false;
    }
    d->marker_hit = false;
    for (int i = 0; i < d->ncomp; i++) d->comp[i].dc_pred = 0;
    return true;
}

static bool jpeg_scan(JpegDecoder *d) {
    int len = jpeg_u16(d);
    if (len < 3 || d->pos + (size_t)len - 2 > d->length) return false;
    int ns = d->data[d->pos++];
    if (ns < 1 || ns > d->ncomp || len != 6 + 2 * ns) return false;
    JpegComponent *scan[JPEG_MAX_COMPONENTS];
    for (int i = 0; i < ns; i++) {
        int id = d->data[d->pos++], tables = d->data[d->pos++];
        scan[i] = NULL;
        for (int c = 0; c < d->ncomp; c++) if (d->comp[c].id == id) scan[i] = &d->comp[c];
        if (!scan[i]) return false;
        scan[i]->td = tables >> 4; scan[i]->ta = tables & 15;
        if (scan[i]->td > 3 || scan[i]->ta > 3 || !d->dc[scan[i]->td].present || !d->ac[scan[i]->ta].present ||
            !d->quant_present[scan[i]->tq]) return false;
    }
    int ss = d->data[d->pos++], se = d->data[d->pos++], ahal = d->data[d->pos++];
    if (ss != 0 || se != 63 || ahal != 0) return false;   // sequential only
    d->bits = 0; d->bit_count = 0; d->marker_hit = false;
    for (int i = 0; i < ns; i++) scan[i]->dc_pred = 0;
    int restarts = 0;
    if (ns == 1) {
        JpegComponent *c = scan[0];
        int bw = (c->ds_w + 7) / 8, bh = (c->ds_h + 7) / 8;
        for (int by = 0; by < bh; by++) for (int bx = 0; bx < bw; bx++) {
            if (d->restart_interval && restarts == d->restart_interval) { if (!jpeg_restart(d)) return false; restarts = 0; }
            if (!jpeg_decode_block(d, c, bx, by)) return false;
            restarts++;
        }
    } else {
        for (int my = 0; my < d->mcuy; my++) for (int mx = 0; mx < d->mcux; mx++) {
            if (d->restart_interval && restarts == d->restart_interval) { if (!jpeg_restart(d)) return false; restarts = 0; }
            for (int i = 0; i < ns; i++) {
                JpegComponent *c = scan[i];
                for (int v = 0; v < c->v; v++) for (int h = 0; h < c->h; h++)
                    if (!jpeg_decode_block(d, c, mx * c->h + h, my * c->v + v)) return false;
            }
            restarts++;
        }
    }
    // Resume marker parsing after the entropy-coded segment.
    while (d->pos + 1 < d->length && !(d->data[d->pos] == 0xFF && d->data[d->pos + 1] != 0x00 &&
           !(d->data[d->pos + 1] >= 0xD0 && d->data[d->pos + 1] <= 0xD7))) d->pos++;
    return true;
}

static const uint8_t *jpeg_row(const JpegComponent *c, int y) {
    if (y < 0) y = 0;
    if (y >= c->ds_h) y = c->ds_h - 1;   // jdmainct context-row replication
    return c->plane + (size_t)y * c->blocks_w * 8;
}

// Produces one full-resolution row of component c for output row y.
static void jpeg_upsample_row(const JpegDecoder *d, const JpegComponent *c, int y, uint8_t *out) {
    int hr = d->hmax / c->h, vr = d->vmax / c->v, w = c->ds_w;
    if (hr == 1 && vr == 1) { memcpy(out, jpeg_row(c, y), (size_t)w); return; }
    if (hr == 2 && vr == 1 && w > 2) {          // h2v1_fancy_upsample
        const uint8_t *in = jpeg_row(c, y);
        int v = in[0];
        *out++ = (uint8_t)v;
        *out++ = (uint8_t)((v * 3 + in[1] + 2) >> 2);
        for (int x = 1; x < w - 1; x++) {
            v = in[x] * 3;
            *out++ = (uint8_t)((v + in[x - 1] + 1) >> 2);
            *out++ = (uint8_t)((v + in[x + 1] + 2) >> 2);
        }
        v = in[w - 1];
        *out++ = (uint8_t)((v * 3 + in[w - 2] + 1) >> 2);
        *out++ = (uint8_t)v;
        return;
    }
    if (hr == 2 && vr == 2 && w > 2) {          // h2v2_fancy_upsample
        int row = y / 2;
        const uint8_t *in0 = jpeg_row(c, row);
        const uint8_t *in1 = jpeg_row(c, (y & 1) ? row + 1 : row - 1);
        int thiscolsum = in0[0] * 3 + in1[0];
        int nextcolsum = in0[1] * 3 + in1[1];
        int lastcolsum;
        *out++ = (uint8_t)((thiscolsum * 4 + 8) >> 4);
        *out++ = (uint8_t)((thiscolsum * 3 + nextcolsum + 7) >> 4);
        lastcolsum = thiscolsum; thiscolsum = nextcolsum;
        for (int x = 2; x < w; x++) {
            nextcolsum = in0[x] * 3 + in1[x];
            *out++ = (uint8_t)((thiscolsum * 3 + lastcolsum + 8) >> 4);
            *out++ = (uint8_t)((thiscolsum * 3 + nextcolsum + 7) >> 4);
            lastcolsum = thiscolsum; thiscolsum = nextcolsum;
        }
        *out++ = (uint8_t)((thiscolsum * 3 + lastcolsum + 8) >> 4);
        *out++ = (uint8_t)((thiscolsum * 4 + 7) >> 4);
        return;
    }
    const uint8_t *in = jpeg_row(c, y / vr);   // h2v1/h2v2/int_upsample replication
    for (int x = 0; x < w; x++) for (int i = 0; i < hr; i++) *out++ = in[x];
}

bool jpeg_decode(const uint8_t *data, size_t length, int32_t *out_width, int32_t *out_height, uint32_t **out_pixels) {
    if (!data || length < 4 || data[0] != 0xFF || data[1] != 0xD8 || !out_pixels) return false;
    JpegDecoder *d = (JpegDecoder *)calloc(1, sizeof(JpegDecoder));
    if (!d) return false;
    d->data = data; d->length = length; d->pos = 2;
    jpeg_prepare_range_limit(d);
    bool ok = false, have_frame = false, have_scan = false;
    uint32_t *pixels = NULL;
    while (d->pos + 4 <= d->length) {
        if (d->data[d->pos] != 0xFF) { d->pos++; continue; }
        int marker = d->data[d->pos + 1];
        d->pos += 2;
        if (marker == 0xFF || (marker >= 0xD0 && marker <= 0xD7) || marker == 0x01) { if (marker == 0xFF) d->pos--; continue; }
        if (marker == 0xD9) break;
        if (marker == 0xDA) {
            if (!have_frame || !jpeg_scan(d)) goto done;
            have_scan = true;
            continue;
        }
        int len = jpeg_u16(d);
        if (len < 2 || d->pos + (size_t)len - 2 > d->length) goto done;
        size_t end = d->pos + (size_t)len - 2;
        if (marker == 0xDB) {
            while (d->pos < end) {
                int pq = d->data[d->pos] >> 4, tq = d->data[d->pos] & 15;
                d->pos++;
                if (tq > 3 || pq > 1 || d->pos + (size_t)(pq ? 128 : 64) > end) goto done;
                for (int i = 0; i < 64; i++) {
                    int v = pq ? (d->data[d->pos] << 8) | d->data[d->pos + 1] : d->data[d->pos];
                    d->pos += pq ? 2 : 1;
                    d->quant[tq][jpeg_natural_order[i]] = v;
                }
                d->quant_present[tq] = true;
            }
        } else if (marker == 0xC4) {
            while (d->pos < end) {
                int tc = d->data[d->pos] >> 4, th = d->data[d->pos] & 15;
                d->pos++;
                if (tc > 1 || th > 3 || d->pos + 16 > end) goto done;
                uint8_t counts[16]; int total = 0;
                for (int i = 0; i < 16; i++) { counts[i] = d->data[d->pos++]; total += counts[i]; }
                if (total > 256 || d->pos + (size_t)total > end) goto done;
                if (!jpeg_build_huffman(tc ? &d->ac[th] : &d->dc[th], counts, d->data + d->pos, total)) goto done;
                d->pos += (size_t)total;
            }
        } else if (marker == 0xDD) {
            if (len != 4) goto done;
            d->restart_interval = jpeg_u16(d);
        } else if (marker == 0xC0 || marker == 0xC1) {
            if (have_frame || len < 8) goto done;
            int precision = d->data[d->pos];
            d->height = (d->data[d->pos + 1] << 8) | d->data[d->pos + 2];
            d->width = (d->data[d->pos + 3] << 8) | d->data[d->pos + 4];
            d->ncomp = d->data[d->pos + 5];
            d->pos += 6;
            if (precision != 8 || d->width <= 0 || d->height <= 0 || (d->ncomp != 1 && d->ncomp != 3) ||
                len != 8 + 3 * d->ncomp) goto done;
            d->hmax = d->vmax = 1;
            for (int i = 0; i < d->ncomp; i++) {
                JpegComponent *c = &d->comp[i];
                c->id = d->data[d->pos]; c->h = d->data[d->pos + 1] >> 4; c->v = d->data[d->pos + 1] & 15; c->tq = d->data[d->pos + 2];
                d->pos += 3;
                if (c->h < 1 || c->h > 4 || c->v < 1 || c->v > 4 || c->tq > 3) goto done;
                if (c->h > d->hmax) d->hmax = c->h;
                if (c->v > d->vmax) d->vmax = c->v;
            }
            d->mcux = (d->width + 8 * d->hmax - 1) / (8 * d->hmax);
            d->mcuy = (d->height + 8 * d->vmax - 1) / (8 * d->vmax);
            for (int i = 0; i < d->ncomp; i++) {
                JpegComponent *c = &d->comp[i];
                if (d->hmax % c->h || d->vmax % c->v) goto done;
                c->ds_w = (d->width * c->h + d->hmax - 1) / d->hmax;
                c->ds_h = (d->height * c->v + d->vmax - 1) / d->vmax;
                c->blocks_w = d->mcux * c->h;
                c->blocks_h = d->mcuy * c->v;
                c->plane = (uint8_t *)calloc((size_t)c->blocks_w * 8 * c->blocks_h * 8, 1);
                if (!c->plane) goto done;
            }
            have_frame = true;
        } else if (marker >= 0xC2 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
            goto done;   // progressive/lossless/arithmetic frames are not used by title.dat
        }
        d->pos = end;
    }
    if (!have_frame || !have_scan) goto done;
    pixels = (uint32_t *)malloc((size_t)d->width * d->height * sizeof(uint32_t));
    uint8_t *rows = (uint8_t *)malloc((size_t)d->ncomp * (d->width + 8 * d->hmax));
    if (!pixels || !rows) { free(rows); free(pixels); pixels = NULL; goto done; }
    int cr_r[256], cb_b[256]; int32_t cr_g[256], cb_g[256];
    for (int i = 0; i < 256; i++) {   // jdcolor.c build_ycc_rgb_table
        int32_t x = i - 128;
        cr_r[i] = (int)(((int32_t)(1.40200 * 65536 + 0.5) * x + 32768) >> 16);
        cb_b[i] = (int)(((int32_t)(1.77200 * 65536 + 0.5) * x + 32768) >> 16);
        cr_g[i] = -(int32_t)(0.71414 * 65536 + 0.5) * x;
        cb_g[i] = -(int32_t)(0.34414 * 65536 + 0.5) * x + 32768;
    }
    const uint8_t *limit = d->range_base + 256;
    size_t row_stride = (size_t)d->width + 8 * d->hmax;
    for (int y = 0; y < d->height; y++) {
        for (int i = 0; i < d->ncomp; i++) jpeg_upsample_row(d, &d->comp[i], y, rows + i * row_stride);
        uint32_t *dst = pixels + (size_t)y * d->width;
        for (int x = 0; x < d->width; x++) {
            int yy = rows[x], r, g, b;
            if (d->ncomp == 1) { r = g = b = yy; }
            else {
                int cb = rows[row_stride + x], cr = rows[2 * row_stride + x];
                r = limit[yy + cr_r[cr]];
                g = limit[yy + (int)((cb_g[cb] + cr_g[cr]) >> 16)];
                b = limit[yy + cb_b[cb]];
            }
            dst[x] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
    free(rows);
    ok = true;
done:
    for (int i = 0; i < JPEG_MAX_COMPONENTS; i++) free(d->comp[i].plane);
    if (ok) { *out_width = d->width; *out_height = d->height; *out_pixels = pixels; }
    else free(pixels);
    free(d);
    return ok;
}
