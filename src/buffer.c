#include "buffer.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void buffer_push_to_row_list(row_list_t *rowList, int row) {
    rowList->items[rowList->count] = row;
    rowList->count ++;
}

void buffer_expand_gap(text_buffer_t *buffer, size_t minimum_required_space) {
    size_t new_total_cap = max(buffer->capacity * 2, buffer->capacity + minimum_required_space);
    buffer->input = realloc(buffer->input, new_total_cap);
    size_t right_text_size = buffer->capacity - buffer->gapEnd;
    memmove(buffer->input + (new_total_cap - right_text_size), buffer->input + buffer->gapEnd, right_text_size);
    buffer->gapEnd = new_total_cap - right_text_size;
    buffer->capacity = new_total_cap;
}

void buffer_insert_bytes(text_buffer_t *buffer, const char *data, size_t size) {
    size_t available_space = buffer->gapEnd - buffer->gapStart;
    if (available_space < size) {
        buffer_expand_gap(buffer, size);
    }
    memcpy(
        buffer->input + buffer->gapStart, 
        data, 
        size
    );
    buffer->gapStart += size;
    buffer->size += size;
    buffer->selectionAnchor = buffer->gapStart;
    buffer->isSaved = false;
}

int buffer_get_previous_char_size(text_buffer_t *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int charSize = 1;
    currentOffset--;

    while (currentOffset > 0 && (buffer_get_char_at(buffer, currentOffset) & 0xC0) == 0x80 ) {
        charSize ++;
        currentOffset--;
    }

    return charSize;
}

void buffer_delete_range(text_buffer_t *buffer, int start, int end) {
    if (start >= end) return;

    int count = end - start;
    buffer_move_gap(buffer, end);
    buffer->gapStart -= count;
    buffer->selectionAnchor = buffer->gapStart; 
    buffer->size -= count;

    buffer->isSaved = false;
}

void buffer_delete_character(text_buffer_t *buffer) {
    if (buffer->gapStart <= 0) return;

    int bytesToDelete = buffer_get_previous_char_size(buffer, buffer->gapStart);

    buffer_delete_range(buffer, buffer->gapStart - bytesToDelete, buffer->gapStart);
}

int buffer_get_line_start(text_buffer_t *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int scanIndex = currentOffset;

    while(scanIndex > 0) {
        scanIndex --;
        if (buffer_get_char_at(buffer, scanIndex) == '\n') {
            return scanIndex + 1;
        }
    }

    return 0;
}

int buffer_get_line_end(text_buffer_t *buffer, int currentOffset) {
    if (currentOffset < 0) return 0;
    if ((int) buffer->size < currentOffset) return 0;

    int scanIndex = currentOffset;

    while(scanIndex < (int) buffer->size) {
        if (buffer_get_char_at(buffer, scanIndex) == '\n') {
            return scanIndex;
        }
        scanIndex ++;
    }

    return buffer->size;
}

int buffer_get_word_start(text_buffer_t *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int scanIndex = currentOffset;

    while(scanIndex > 0 && (buffer_get_char_at(buffer, scanIndex - 1) == ' ' || buffer_get_char_at(buffer, scanIndex - 1) == '\n')) {
        scanIndex --;
    }

    while(scanIndex > 0 && (buffer_get_char_at(buffer, scanIndex - 1) != ' ' && buffer_get_char_at(buffer, scanIndex - 1) != '\n')) {
        scanIndex --;
    }

    return scanIndex;
}

int buffer_get_word_end(text_buffer_t *buffer, int currentOffset) {
    if (currentOffset < 0) return 0;
    if ((int) buffer->size < currentOffset) return 0;

    int scanIndex = currentOffset;

    if (scanIndex < (int) buffer->size && buffer_get_char_at(buffer, scanIndex) == '\n') {
        return scanIndex + 1;
    }

    while(scanIndex < (int) buffer->size && (buffer_get_char_at(buffer, scanIndex) != ' ' && buffer_get_char_at(buffer, scanIndex) != '\n')) {
        scanIndex ++;
    }

    while(scanIndex < (int) buffer->size && (buffer_get_char_at(buffer, scanIndex) == ' ' || buffer_get_char_at(buffer, scanIndex) == '\t')) {
        scanIndex ++;
    }

    return scanIndex;
}

//copied from raylib's implementation
const char *buffer_codepoint_to_utf8(int codepoint, int *utf8Size) {
    static char utf8[6] = { 0 };
    memset(utf8, 0, 6); // Clear static array
    int size = 0;       // Byte size of codepoint

    if (codepoint <= 0x7f)
    {
        utf8[0] = (char)codepoint;
        size = 1;
    }
    else if (codepoint <= 0x7ff)
    {
        utf8[0] = (char)(((codepoint >> 6) & 0x1f) | 0xc0);
        utf8[1] = (char)((codepoint & 0x3f) | 0x80);
        size = 2;
    }
    else if (codepoint <= 0xffff)
    {
        utf8[0] = (char)(((codepoint >> 12) & 0x0f) | 0xe0);
        utf8[1] = (char)(((codepoint >>  6) & 0x3f) | 0x80);
        utf8[2] = (char)((codepoint & 0x3f) | 0x80);
        size = 3;
    }
    else if (codepoint <= 0x10ffff)
    {
        utf8[0] = (char)(((codepoint >> 18) & 0x07) | 0xf0);
        utf8[1] = (char)(((codepoint >> 12) & 0x3f) | 0x80);
        utf8[2] = (char)(((codepoint >>  6) & 0x3f) | 0x80);
        utf8[3] = (char)((codepoint & 0x3f) | 0x80);
        size = 4;
    }

    *utf8Size = size;

    return utf8;
}

void buffer_insert_character(text_buffer_t *buffer, int key) {
    int byteSize = 0;
    const char *utf8Symbol = buffer_codepoint_to_utf8(key, &byteSize);
    buffer_insert_bytes(buffer, utf8Symbol, byteSize);
}

void buffer_move_gap(text_buffer_t *buffer, int new_offset) {
    if (new_offset < buffer->gapStart) {
        int size = buffer->gapStart - new_offset;
        memmove(buffer->input + buffer->gapEnd - size, buffer->input + new_offset, size);
        buffer->gapStart -= size;
        buffer->gapEnd -= size;
    }

    if (new_offset > buffer->gapStart) {
        int size = new_offset - buffer->gapStart;
        memmove(buffer->input + buffer->gapStart, buffer->input + buffer->gapEnd, size);
        buffer->gapStart += size;
        buffer->gapEnd += size;
    }
}

char buffer_get_char_at(text_buffer_t *buffer, int logical_index) {
    if (logical_index < buffer->gapStart) {
        return buffer->input[logical_index];
    } else {
        return buffer->input[logical_index + (buffer->gapEnd - buffer->gapStart)];
    }
}

char *buffer_get_text(text_buffer_t *buffer, arena_t *arena) {
    char *str = (char *) arena_alloc(arena, buffer->size + 1);
    memcpy(str, buffer->input, buffer->gapStart);
    memcpy(str + buffer->gapStart, buffer->input + buffer->gapEnd, buffer->size - buffer->gapStart);

    str[buffer->size] = '\0';
    return str;
}

int buffer_find_text(text_buffer_t *buffer, const char *query, int start_offset) {
    int query_length = strlen(query);

    for (int i = start_offset; i <= (int) buffer->size - query_length; i++) {
        bool matched = true;
        for (int j = 0; j < query_length; j++) {
            if (query[j] != buffer_get_char_at(buffer, i + j)) {
                matched = false;
                break;
            }
        }
        if (matched) return i;
    }
    return -1;
}

void buffer_read_utf8_sequence(text_buffer_t *buffer, int index, char *out) {
    for (int i = 0; i < 5; i++) {
        out[i] = '\0';
    }

    for (int i = 0; i < 4; i++) {
        if ((index + i) >= (int) buffer->size) break;
        out[i] = buffer_get_char_at(buffer, index + i);
    }
}
