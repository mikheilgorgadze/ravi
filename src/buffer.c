#include "buffer.h"
#include <assert.h>
#include <string.h>

unsigned char* ArenaAlloc(Arena *arena, size_t size) {
    assert(arena->used + size <= arena->capacity);

    void *ptr = arena->memory + arena->used;
    arena->used += size;

    return ptr;
}

void PushRowStarts(RowList *rowList, int row) {
    rowList->items[rowList->count] = row;
    rowList->count ++;
}


void InsertBytes(TextBuffer *buffer, const char *data, size_t size) {
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
    }

}

int GetPreviousCharSize(char *buffer, int currentOffset) {
    if (currentOffset <= 0) return 0;

    int charSize = 1;
    currentOffset--;

    while (currentOffset > 0 && (buffer[currentOffset] & 0xC0) == 0x80 ) {
        charSize ++;
        currentOffset--;
    }

    return charSize;
}

void DeleteRange(TextBuffer *buffer, int start, int end) {
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
}

void DeleteCharacter(TextBuffer *buffer) {
    if (buffer->cursorByteOffset <= 0) return;

    int bytesToDelete = GetPreviousCharSize(buffer->input, buffer->cursorByteOffset);

    DeleteRange(buffer, buffer->cursorByteOffset - bytesToDelete, buffer->cursorByteOffset);
}

size_t string_len_utf8(const char *str) {
    size_t count = 0;
    while (*str) {
        if ((*str & 0xC0) != 0x80 ) {
            count++;
        }
        str++;
    }
    return count;
}


size_t safe_strlen(const char *s, size_t max_len) {
    size_t length = 0;
    if (s == NULL) { // Check for null pointer
        return 0;
    }
    while (length < max_len && s[length] != '\0') { // Check bounds and terminator
        length++;
    }
    return length;
}


int GetLineStart(char *buffer, int currentOffset) {
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

int GetLineEnd(char *buffer, int currentOffset, size_t bufferSize) {
    if (currentOffset < 0) return 0;
    if (bufferSize < currentOffset) return 0;

    int scanIndex = currentOffset;

    while(scanIndex < bufferSize) {
        if (buffer[scanIndex] == '\n') {
            return scanIndex;
        }
        scanIndex ++;
    }

    return bufferSize;
}

int GetWordStart(char *buffer, int currentOffset) {
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

int GetWordEnd(char *buffer, int currentOffset, size_t bufferSize) {
    if (currentOffset < 0) return 0;
    if (bufferSize < currentOffset) return 0;

    int scanIndex = currentOffset;

    if (scanIndex < bufferSize && buffer[scanIndex] == '\n') {
        return scanIndex + 1;
    }

    while(scanIndex < bufferSize && (buffer[scanIndex] != ' ' && buffer[scanIndex] != '\n')) {
        scanIndex ++;
    }

    while(scanIndex < bufferSize && (buffer[scanIndex] == ' ' || buffer[scanIndex] == '\t')) {
        scanIndex ++;
    }

    return scanIndex;
}

//copied from raylib's implementation
const char *CodepointToUTF8_Buffer(int codepoint, int *utf8Size) {
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

void InsertCharacter(TextBuffer *buffer, int key) {
    int byteSize = 0;
    const char *utf8Symbol = CodepointToUTF8_Buffer(key, &byteSize);
    InsertBytes(buffer, utf8Symbol, byteSize);
}

