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
    int32_t **shared_refs;
    size_t shared_count,shared_capacity;
} Arena;

Arena arena_create(size_t capacity);
void arena_free(Arena *arena);
void *arena_alloc(Arena *arena, size_t size);
void *arena_alloc_zero(Arena *arena, size_t size);
char *arena_strdup(Arena *arena, const char *str);
void arena_reset(Arena *arena);
// Retain a heap reference-counted int array until reset/free (counter at [0]).
bool arena_retain_shared(Arena *arena,int32_t *storage);

// Global arenas used throughout client
extern Arena g_arena_perm;   // Static cache, configs, fonts
extern Arena g_arena_world;  // Terrain and loaded chunk models
extern Arena g_arena_frame;  // Per-frame scratch memory (cleared every 20ms)

#endif // RS2_ARENA_H
