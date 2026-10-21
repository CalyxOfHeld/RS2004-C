#include "core/jagfile.h"
#include "core/packet.h"
#include "core/bzip2.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int32_t jagfile_gen_hash(const char *name) {
    int32_t hash = 0;
    for (size_t i = 0; name[i] != '\0'; i++) {
        char c = (char)toupper((unsigned char)name[i]);
        hash = hash * 61 + c - 32;
    }
    return hash;
}

Jagfile *jagfile_load(const uint8_t *src, size_t src_len, Arena *arena) {
    if (!src || src_len < 6) {
        return NULL;
    }

    Jagfile *jag = arena ? (Jagfile *)arena_alloc_zero(arena, sizeof(Jagfile))
                         : (Jagfile *)calloc(1, sizeof(Jagfile));
    if (!jag) return NULL;

    Packet pkt = packet_wrap((uint8_t *)src, src_len);
    int32_t unpacked_size = g3(&pkt);
    int32_t packed_size = g3(&pkt);

    if (packed_size == unpacked_size) {
        jag->buffer = (uint8_t *)src;
        jag->unpacked = false;
    } else {
        uint8_t *decompressed = arena ? (uint8_t *)arena_alloc(arena, unpacked_size)
                                      : (uint8_t *)malloc(unpacked_size);
        if (!decompressed) {
            if (!arena) free(jag);
            return NULL;
        }

        bzip_decompress((int8_t *)decompressed, (int8_t *)src, packed_size, 6);
        jag->buffer = decompressed;
        jag->unpacked = true;
        pkt = packet_wrap(decompressed, unpacked_size);
    }

    jag->file_count = g2(&pkt);
    size_t count = (size_t)jag->file_count;

    jag->file_hash          = arena ? (int32_t *)arena_alloc(arena, count * sizeof(int32_t))
                                    : (int32_t *)malloc(count * sizeof(int32_t));
    jag->file_unpacked_size = arena ? (int32_t *)arena_alloc(arena, count * sizeof(int32_t))
                                    : (int32_t *)malloc(count * sizeof(int32_t));
    jag->file_packed_size   = arena ? (int32_t *)arena_alloc(arena, count * sizeof(int32_t))
                                    : (int32_t *)malloc(count * sizeof(int32_t));
    jag->file_offset        = arena ? (int32_t *)arena_alloc(arena, count * sizeof(int32_t))
                                    : (int32_t *)malloc(count * sizeof(int32_t));

    int32_t cur_offset = (int32_t)(pkt.pos + count * 10);
    for (size_t i = 0; i < count; i++) {
        jag->file_hash[i] = g4(&pkt);
        jag->file_unpacked_size[i] = g3(&pkt);
        jag->file_packed_size[i] = g3(&pkt);
        jag->file_offset[i] = cur_offset;
        cur_offset += jag->file_packed_size[i];
    }

    return jag;
}

Jagfile *jagfile_load_file(const char *filepath, Arena *arena) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "[Jagfile] Failed to open file: %s\n", filepath);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return NULL;
    }

    uint8_t *bytes = arena ? (uint8_t *)arena_alloc(arena, size)
                           : (uint8_t *)malloc(size);
    if (!bytes) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(bytes, 1, size, f);
    fclose(f);

    if (read_bytes != (size_t)size) {
        if (!arena) free(bytes);
        return NULL;
    }

    return jagfile_load(bytes, (size_t)size, arena);
}

uint8_t *jagfile_read(Jagfile *jag, const char *name, uint8_t *dst, size_t *out_size, Arena *arena) {
    if (!jag || !name) return NULL;

    int32_t hash = jagfile_gen_hash(name);

    for (int32_t i = 0; i < jag->file_count; i++) {
        if (jag->file_hash[i] == hash) {
            size_t unp_size = (size_t)jag->file_unpacked_size[i];
            if (out_size) {
                *out_size = unp_size;
            }

            if (!dst) {
                dst = arena ? (uint8_t *)arena_alloc(arena, unp_size)
                            : (uint8_t *)malloc(unp_size);
                if (!dst) return NULL;
            }

            if (jag->unpacked) {
                memcpy(dst, jag->buffer + jag->file_offset[i], unp_size);
            } else {
                bzip_decompress((int8_t *)dst, (int8_t *)jag->buffer, jag->file_packed_size[i], jag->file_offset[i]);
            }

            return dst;
        }
    }

    return NULL;
}

void jagfile_free(Jagfile *jag) {
    if (jag) {
        free(jag->file_offset);
        free(jag->file_packed_size);
        free(jag->file_unpacked_size);
        free(jag->file_hash);
        if (jag->unpacked) {
            free(jag->buffer);
        }
        free(jag);
    }
}
