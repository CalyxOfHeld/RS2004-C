#ifndef RS2_ARENA_H
#define RS2_ARENA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
    size_t peak;
} Arena;

Arena arena_create(size_t capacity);
void arena_free(Arena *arena);
void *arena_alloc(Arena *arena, size_t size);
void *arena_alloc_zero(Arena *arena, size_t size);
char *arena_strdup(Arena *arena, const char *str);
void arena_reset(Arena *arena);

// Global arenas used throughout client
extern Arena g_arena_perm;   // Static cache, configs, fonts
extern Arena g_arena_world;  // Terrain and loaded chunk models
extern Arena g_arena_frame;  // Per-frame scratch memory (cleared every 20ms)

#endif // RS2_ARENA_H
