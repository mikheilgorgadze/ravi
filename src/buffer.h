#ifndef BUFFER_H
#define BUFFER_H

#include "arena.h"
#include <stddef.h>
#include <stdbool.h>


typedef struct {
    int *items;
    int count;
} row_list_t;

typedef struct {
    char* input;
    int gapStart;
    int gapEnd;
    size_t size;
    size_t capacity;
    int selectionAnchor;
    bool isSaved;
    row_list_t rowList;
} text_buffer_t;

void buffer_push_to_row_list(row_list_t *rowList, int row);
void buffer_insert_bytes(text_buffer_t *buffer, const char *data, size_t size);
int  buffer_get_previous_char_size(text_buffer_t *buffer, int currentOffset);
void buffer_delete_range(text_buffer_t *buffer, int start, int end);
void buffer_delete_character(text_buffer_t *buffer);
int  buffer_get_line_start(text_buffer_t *buffer, int currentOffset);
int  buffer_get_line_end(text_buffer_t *buffer, int currentOffset);
int  buffer_get_word_start(text_buffer_t *buffer, int currentOffset);
int  buffer_get_word_end(text_buffer_t *buffer, int currentOffset);
const char *buffer_codepoint_to_utf8(int codepoint, int *utf8Size);
void buffer_insert_character(text_buffer_t *buffer, int key);
void buffer_move_gap(text_buffer_t *buffer, int new_offset);
void buffer_expand_gap(text_buffer_t *buffer, size_t minimum_required_space);
char buffer_get_char_at(text_buffer_t *buffer, int logical_index);
char *buffer_get_text(text_buffer_t *buffer, arena_t *arena);
int buffer_find_text(text_buffer_t *buffer, const char *query, int start_offset);
void buffer_read_utf8_sequence(text_buffer_t *buffer, int index, char *out);

#endif
