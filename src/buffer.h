#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>


typedef struct {
    int *items;
    int count;
} row_list_t;

typedef struct {
    char* input;
    size_t size;
    size_t capacity;
    int cursorByteOffset;
    int selectionAnchor;
    bool isSaved;
    row_list_t rowList;
} text_buffer_t;

void buffer_push_to_row_list(row_list_t *rowList, int row);
void buffer_insert_bytes(text_buffer_t *buffer, const char *data, size_t size);
int  buffer_get_previous_char_size(char *buffer, int currentOffset);
void buffer_delete_range(text_buffer_t *buffer, int start, int end);
void buffer_delete_character(text_buffer_t *buffer);
int  buffer_get_line_start(char *buffer, int currentOffset);
int  buffer_get_line_end(char *buffer, int currentOffset, size_t bufferSize);
int  buffer_get_word_start(char *buffer, int currentOffset);
int  buffer_get_word_end(char *buffer, int currentOffset, size_t bufferSize);
const char *buffer_codepoint_to_utf8(int codepoint, int *utf8Size);
void buffer_insert_character(text_buffer_t *buffer, int key);

#endif
