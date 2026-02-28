#include "buffer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void buffer_push_to_row_list(row_list_t *rowList, int row) {
    rowList->items[rowList->count] = row;
    rowList->count ++;
}

void buffer_insert_bytes(text_buffer_t *buffer, const char *data, size_t size) {
    if (buffer->size + size < buffer->capacity) {
        memmove(
            buffer->input + buffer->cursorByteOffset + size,
            buffer->input + buffer->cursorByteOffset, 
            buffer->size - buffer->cursorByteOffset + 1
        );
        memcpy(
            &buffer->input[buffer->cursorByteOffset], 
            data, 
            size
        );
        buffer->cursorByteOffset += size;
        buffer->size += size;
        buffer->selectionAnchor = buffer->cursorByteOffset;

        buffer->isSaved = false;
        buffer->input[buffer->size] = '\0';
    }

}

int buffer_get_previous_char_size(char *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int charSize = 1;
    currentOffset--;

    while (currentOffset > 0 && (buffer[currentOffset] & 0xC0) == 0x80 ) {
        charSize ++;
        currentOffset--;
    }

    return charSize;
}

void buffer_delete_range(text_buffer_t *buffer, int start, int end) {
    if (start >= end)return;

    int count = end - start;

    memmove(
        buffer->input + start,
        buffer->input + end, 
        buffer->size - end + 1
    );

    buffer->cursorByteOffset = start;
    buffer->selectionAnchor = buffer->cursorByteOffset; 
    buffer->size -= count;

    buffer->isSaved = false;
    buffer->input[buffer->size] = '\0';
}

void buffer_delete_character(text_buffer_t *buffer) {
    if (buffer->cursorByteOffset <= 0) return;

    int bytesToDelete = buffer_get_previous_char_size(buffer->input, buffer->cursorByteOffset);

    buffer_delete_range(buffer, buffer->cursorByteOffset - bytesToDelete, buffer->cursorByteOffset);
}

int buffer_get_line_start(char *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int scanIndex = currentOffset;

    while(scanIndex > 0) {
        scanIndex --;
        if (buffer[scanIndex] == '\n') {
            return scanIndex + 1;
        }
    }

    return 0;
}

int buffer_get_line_end(char *buffer, int currentOffset, size_t bufferSize) {
    if (currentOffset < 0) return 0;
    if ((int) bufferSize < currentOffset) return 0;

    int scanIndex = currentOffset;

    while(scanIndex < (int) bufferSize) {
        if (buffer[scanIndex] == '\n') {
            return scanIndex;
        }
        scanIndex ++;
    }

    return bufferSize;
}

int buffer_get_word_start(char *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int scanIndex = currentOffset;

    while(scanIndex > 0 && (buffer[scanIndex - 1] == ' ' || buffer[scanIndex - 1] == '\n')) {
        scanIndex --;
    }

    while(scanIndex > 0 && (buffer[scanIndex - 1] != ' ' && buffer[scanIndex - 1] != '\n')) {
        scanIndex --;
    }

    return scanIndex;
}

int buffer_get_word_end(char *buffer, int currentOffset, size_t bufferSize) {
    if (currentOffset < 0) return 0;
    if ((int) bufferSize < currentOffset) return 0;

    int scanIndex = currentOffset;

    if (scanIndex < (int) bufferSize && buffer[scanIndex] == '\n') {
        return scanIndex + 1;
    }

    while(scanIndex < (int) bufferSize && (buffer[scanIndex] != ' ' && buffer[scanIndex] != '\n')) {
        scanIndex ++;
    }

    while(scanIndex < (int) bufferSize && (buffer[scanIndex] == ' ' || buffer[scanIndex] == '\t')) {
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

