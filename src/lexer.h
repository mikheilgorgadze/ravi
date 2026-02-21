#ifndef LEXER_H
#define LEXER_H

#include "buffer.h"
#include <raylib.h>

typedef enum {
    START,
    IN_STRING,
    IN_SINGLE_QUOTES,
    IN_DOUBLE_QUOTES,
    IN_SINGLELINE_COMMENT,
    IN_MULTILINE_COMMENT,
    SPACE,
} TokenState;

typedef struct {
    char *name;
    Color color;
} Keyword;


typedef struct {
    int start;
    int end;
    Keyword keyword;
} SyntaxToken;

typedef struct {
    SyntaxToken *items;
    int count;
} SyntaxTokenList;

void CalculateSyntaxHighlights(TextBuffer *textBuffer, Keyword *keywords, SyntaxTokenList *syntaxTokens);

#endif
