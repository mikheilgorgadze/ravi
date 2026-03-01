#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>

typedef struct arena_node {
    unsigned char *base_memory;
    size_t capacity;
    size_t used;
    struct arena_node *next;
} arena_node_t;

typedef struct {
    arena_node_t *head;
    arena_node_t *current;
    size_t default_capacity;
} arena_t;

void arena_reset(arena_t *arena);
void arena_free(arena_t *arena);
unsigned char* arena_alloc(arena_t *arena, size_t size);
void arena_initialize(arena_t *arena, size_t capacity);

#endif
