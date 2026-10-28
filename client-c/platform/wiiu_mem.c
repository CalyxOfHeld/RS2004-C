// Wii U only (Makefile.wiiu links with -Wl,--wrap=memset,--wrap=memcpy).
//
// October 28 hardware profile: memset and memcpy took 10% and 6% of an in-game
// frame (frame clears, calloc, model copies); memcpy ran at 99 MB/s against
// memset's 550 MB/s. Large aligned blocks are done a 32-byte cache line at a
// time: dcbz allocates a destination line without reading it from memory, and
// dcbt prefetches the source ahead. Everything else goes to newlib.
#include <stddef.h>
#include <stdint.h>

void *__real_memset(void *dst, int c, size_t n);
void *__real_memcpy(void *dst, const void *src, size_t n);
void *__wrap_memset(void *dst, int c, size_t n);
void *__wrap_memcpy(void *dst, const void *src, size_t n);

enum { LINE = 32, PREFETCH = 4 * LINE, MIN_BLOCK = 256 };

static inline void line_zero(void *p) { __asm__ volatile("dcbz 0,%0" : : "r"(p) : "memory"); }
static inline void line_touch(const void *p) { __asm__ volatile("dcbt 0,%0" : : "r"(p)); }

void *__wrap_memset(void *dst, int c, size_t n) {
    uint8_t *d = dst;
    if (n < MIN_BLOCK) return __real_memset(dst, c, n);
    size_t head = (size_t)(-(uintptr_t)d & (LINE - 1));
    __real_memset(d, c, head);
    d += head; n -= head;
    uint32_t v = (uint8_t)c * 0x01010101u;
    for (; n >= LINE; d += LINE, n -= LINE) {
        line_zero(d);
        if (v) {
            uint32_t *w = (uint32_t *)d;
            w[0] = v; w[1] = v; w[2] = v; w[3] = v; w[4] = v; w[5] = v; w[6] = v; w[7] = v;
        }
    }
    __real_memset(d, c, n);
    return dst;
}

void *__wrap_memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    // Line copies need the source word-aligned once the destination is line-aligned.
    // dcbz clears a destination line before it is read, so overlapping (invalid
    // for memcpy, but harmless in newlib) ranges go to newlib too.
    if (n < MIN_BLOCK || (((uintptr_t)d ^ (uintptr_t)s) & 3) || (d < s + n && s < d + n))
        return __real_memcpy(dst, src, n);
    size_t head = (size_t)(-(uintptr_t)d & (LINE - 1));
    __real_memcpy(d, s, head);
    d += head; s += head; n -= head;
    for (; n >= LINE; d += LINE, s += LINE, n -= LINE) {
        line_touch(s + PREFETCH);
        line_zero(d);
        const uint32_t *r = (const uint32_t *)s;
        uint32_t *w = (uint32_t *)d;
        uint32_t a = r[0], b = r[1], c = r[2], e = r[3], f = r[4], g = r[5], h = r[6], i = r[7];
        w[0] = a; w[1] = b; w[2] = c; w[3] = e; w[4] = f; w[5] = g; w[6] = h; w[7] = i;
    }
    __real_memcpy(d, s, n);
    return dst;
}
