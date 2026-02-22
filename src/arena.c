#include "arena.h"
#include <assert.h>

unsigned char* arena_alloc(arena_t *arena, size_t size) {
    assert(arena->used + size <= arena->capacity);

    void *ptr = arena->base_memory + arena->used;
    arena->used += size;

    return ptr;
}

void arena_free(arena_t *arena) {
    free(arena->base_memory);
    arena->base_memory = NULL;
    arena->capacity = 0;
    arena->used = 0;
}

void arena_reset(arena_t *arena) {
    arena->used = 0;
}
