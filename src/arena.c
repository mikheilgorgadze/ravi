#include "arena.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>

unsigned char* arena_alloc(arena_t *arena, size_t size) {
    if (arena->current->used + size > arena->current->capacity) {
        size_t new_capacity = max(arena->default_capacity, size);
        if (arena->current->next == NULL) {
            arena_node_t *next = malloc(sizeof(arena_node_t));
            next->base_memory = malloc(new_capacity);
            next->used = 0;
            next->next = NULL;
            next->capacity = new_capacity;
            arena->current->next = next;
            arena->current = next;
        } else {
            arena->current = arena->current->next;
            if (arena->current->capacity < size) {
                arena->current->capacity = new_capacity;
                arena->current->base_memory = realloc(arena->current->base_memory, new_capacity);
                arena->current->used = 0;
            }
        }
    }

    void *ptr = arena->current->base_memory + arena->current->used;
    arena->current->used += size;

    return ptr;
}

void arena_free(arena_t *arena) {
    arena_node_t *current = arena->head;
    while(current != NULL) { 
        arena_node_t *temp = current->next;
        free(current->base_memory);
        free(current);
        current = temp;
    }
    arena->head = NULL;
    arena->current = arena->head;
    arena->default_capacity = 0;
}

void arena_reset(arena_t *arena) {
    arena_node_t *current = arena->head;
    while(current != NULL) { 
        current->used = 0;
        current = current->next;
    }
    arena->current = arena->head;
}

void arena_initialize(arena_t *arena, size_t capacity) {
    arena_node_t *arena_node =  malloc(sizeof(arena_node_t));
    arena_node->base_memory = malloc(capacity);
    arena_node->capacity = capacity;
    arena_node->used = 0;
    arena_node->next = NULL;

    arena->head = arena_node;
    arena->current = arena_node;
    arena->default_capacity = capacity;
}
