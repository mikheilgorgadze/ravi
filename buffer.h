#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    unsigned char *memory;
    size_t capacity;
    size_t used;
} Arena;

typedef struct {
    char* input;
    size_t size;
    size_t capacity;
    int cursorByteOffset;
    int selectionAnchor;
    bool isSaved;

    // row count cache
    int *rowStarts;
    int rowCount;
    int rowCapacity;
} TextBuffer;

unsigned char* ArenaAlloc(Arena *arena, size_t size);
void PushRowStarts(TextBuffer *buffer, Arena *arena, int row);
void InsertBytes(TextBuffer *buffer, Arena *arena, const char *data, size_t size);
int GetPreviousCharSize(char *buffer, int currentOffset);
void DeleteRange(TextBuffer *buffer, int start, int end);
void DeleteCharacter(TextBuffer *buffer);
size_t string_len_utf8(const char *str);
size_t safe_strlen(const char *s, size_t max_len);
int GetLineStart(char *buffer, int currentOffset);
int GetLineEnd(char *buffer, int currentOffset, size_t bufferSize);
int GetWordStart(char *buffer, int currentOffset);
int GetWordEnd(char *buffer, int currentOffset, size_t bufferSize);
const char *CodepointToUTF8_Buffer(int codepoint, int *utf8Size);
void InsertCharacter(TextBuffer *buffer, Arena *arena, int key);

#endif
