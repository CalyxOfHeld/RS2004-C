#include "core/arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Arena g_arena_perm = {0};
Arena g_arena_world = {0};
Arena g_arena_frame = {0};

#define ALIGNMENT 8
#define ALIGN_UP(n) (((n) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

Arena arena_create(size_t capacity) {
    Arena arena={0};
    arena.buffer = (uint8_t *)malloc(capacity);
    arena.capacity = capacity;
    arena.offset = 0;
    arena.peak = 0;
    if (!arena.buffer) {
        fprintf(stderr, "[Arena] Failed to allocate %zu bytes\n", capacity);
    }
    return arena;
}

void arena_free(Arena *arena) {
    if (arena) {
        arena_reset(arena);
        free(arena->shared_refs);arena->shared_refs=NULL;arena->shared_capacity=0;
        free(arena->buffer);
        arena->buffer = NULL;
        arena->capacity = 0;
        arena->offset = 0;
        arena->peak = 0;
    }
}

void *arena_alloc(Arena *arena, size_t size) {
    size_t aligned_size = ALIGN_UP(size);
    if (arena->offset + aligned_size > arena->capacity) {
        fprintf(stderr, "[Arena OOM] Requested %zu (aligned %zu), cap %zu, used %zu\n",
                size, aligned_size, arena->capacity, arena->offset);
        return NULL;
    }

    void *ptr = arena->buffer + arena->offset;
    arena->offset += aligned_size;
    if (arena->offset > arena->peak) {
        arena->peak = arena->offset;
    }
    return ptr;
}

void *arena_alloc_zero(Arena *arena, size_t size) {
    void *ptr = arena_alloc(arena, size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

char *arena_strdup(Arena *arena, const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = (char *)arena_alloc(arena, len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

void arena_reset(Arena *arena) {
    if (arena) {
        for(size_t i=0;i<arena->shared_count;i++) if(--arena->shared_refs[i][0]==0)free(arena->shared_refs[i]);
        arena->shared_count=0;
        arena->offset = 0;
    }
}

bool arena_retain_shared(Arena *arena,int32_t *storage) {
    if(arena->shared_count==arena->shared_capacity) {
        size_t capacity=arena->shared_capacity ? arena->shared_capacity*2:32;
        int32_t **refs=realloc(arena->shared_refs,capacity*sizeof(*refs));if(!refs)return false;
        arena->shared_refs=refs;arena->shared_capacity=capacity;
    }
    storage[0]++;arena->shared_refs[arena->shared_count++]=storage;return true;
}
