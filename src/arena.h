#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>

typedef struct {
    unsigned char *base_memory;
    size_t capacity;
    size_t used;
} arena_t;

void arena_reset(arena_t *arena);
void arena_free(arena_t *arena);
unsigned char* arena_alloc(arena_t *arena, size_t size);

#endif
