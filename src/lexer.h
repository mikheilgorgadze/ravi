#ifndef LEXER_H
#define LEXER_H

#include "buffer.h"

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_DATA_TYPE,
    TOKEN_STRING_LITERAL,
    TOKEN_SINGLE_QUOTE,
    TOKEN_CONSTANT,
    TOKEN_MACRO,
    TOKEN_COMMENT,
} token_type_t;

typedef enum {
    START,
    IN_TOKEN,
    IN_SINGLE_QUOTES,
    IN_DOUBLE_QUOTES,
    IN_SINGLELINE_COMMENT,
    IN_MULTILINE_COMMENT,
    SPACE,
} token_state_t;

typedef struct {
    char *name;
    token_type_t token_type;
} keyword_t;


typedef struct {
    int start;
    int end;
    keyword_t keyword;
} syntax_token_t;

typedef struct {
    syntax_token_t *items;
    int count;
} syntax_token_list_t;

void calculate_syntax_highlights(text_buffer_t *textBuffer, keyword_t *keywords, syntax_token_list_t *syntaxTokens, int startIndex, int endIndex);

#endif
