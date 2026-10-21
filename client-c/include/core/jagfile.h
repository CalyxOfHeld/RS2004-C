#ifndef RS2_JAGFILE_H
#define RS2_JAGFILE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "core/arena.h"

typedef struct {
    uint8_t *buffer;
    int32_t file_count;
    int32_t *file_hash;
    int32_t *file_unpacked_size;
    int32_t *file_packed_size;
    int32_t *file_offset;
    bool unpacked;
} Jagfile;

Jagfile *jagfile_load(const uint8_t *src, size_t src_len, Arena *arena);
Jagfile *jagfile_load_file(const char *filepath, Arena *arena);
uint8_t *jagfile_read(Jagfile *jag, const char *name, uint8_t *dst, size_t *out_size, Arena *arena);
void     jagfile_free(Jagfile *jag);

#endif // RS2_JAGFILE_H
