#include "lexer.h"
#include "buffer.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

keyword_t *get_matching_keyword(char *word, keyword_t *keywords) {
    int k = 0;
    while (keywords[k].name != NULL) {
        if (strcmp(word, keywords[k].name) == 0) {
            return &keywords[k];
        }
        k++;
    }
    return NULL;
}

void push_token(syntax_token_list_t *tokens, syntax_token_t newToken) {
    tokens->items[tokens->count] = newToken;
    tokens->count ++;
}

int advance_token_state(text_buffer_t *buffer, int index, token_state_t *current_state) {
    char c = buffer_get_char_at(buffer, index);
    char next_c = (index + 1 < (int) buffer->size) ? buffer_get_char_at(buffer, index + 1) : '\0'; 

    switch (*current_state) {
        case START:
            if (c == '/' && next_c == '*') { *current_state = IN_MULTILINE_COMMENT; return 2; }
            if (c == '/' && next_c == '/') { *current_state = IN_SINGLELINE_COMMENT;  return 2; }
            if (c == '\'') { *current_state = IN_SINGLE_QUOTES; return 1; }
            if (c == '\"') { *current_state = IN_DOUBLE_QUOTES; return 1; }
            if (isalnum(c) || c == '#') { *current_state = IN_TOKEN;  return 1; }
            if (isspace(c)) { *current_state = SPACE;  return 1; }
            break;
        case IN_SINGLE_QUOTES:
            if (c == '\\') { return 2; }
            if (c == '\'') { *current_state = START; return 1; }
            break;
        case IN_DOUBLE_QUOTES:
            if (c == '\\') { return 2; }
            if (c == '\"') { *current_state = START; return 1; }
            break;
        case IN_SINGLELINE_COMMENT:
            if (c == '\n') { *current_state = START; return 1; }
            break;
        case IN_MULTILINE_COMMENT:
            if (c == '*' && next_c == '/') { *current_state = START; return 2; }
            break;
        case IN_TOKEN:
            if (!isalnum(c) && c != '#') { *current_state = START; return 0; }
            break;
        case SPACE:
            if (!isspace(c)) { *current_state = START; return 0; }
            break;
    }

    return 1;
}

void calculate_syntax_highlights(text_buffer_t *textBuffer, keyword_t *keywords, syntax_token_list_t *syntaxTokens, int startIndex, int endIndex){
    token_state_t current_state = START;
    int j = 0;
    while (j < startIndex) {
        j += advance_token_state(textBuffer, j, &current_state);
    }

    char buffer[1024];
    syntax_token_t current_pair;
    int buffer_len = 0;
    syntaxTokens->count = 0;

    int i = startIndex;
    while (i < endIndex) {
        token_state_t prev_state = current_state;
        int advance_count = advance_token_state(textBuffer, i, &current_state);

        if (prev_state == START && current_state != START) {
            current_pair.start = i;
            if (current_state == IN_TOKEN) {
                buffer_len = 0;
            }
        } 

        if (current_state == IN_TOKEN) {
            if (buffer_len < 1023) {
                buffer[buffer_len] = buffer_get_char_at(textBuffer, i);
                buffer_len++;
            }
        } else if (prev_state != current_state && current_state == START) {
            current_pair.end = i + advance_count;
            if (prev_state == IN_TOKEN) {
                buffer[buffer_len] = '\0';
                keyword_t *keyword = get_matching_keyword(buffer, keywords);
                if (keyword != NULL) {
                    current_pair.keyword = *keyword;
                    push_token(syntaxTokens, current_pair);
                }
            } else if (prev_state == IN_SINGLE_QUOTES) {
                buffer[buffer_len] = '\0';
                keyword_t *keyword = get_matching_keyword("single_quotes", keywords);
                if (keyword != NULL) {
                    current_pair.keyword = *keyword;
                    push_token(syntaxTokens, current_pair);
                }
            } else if (prev_state == IN_DOUBLE_QUOTES) {
                buffer[buffer_len] = '\0';
                keyword_t *keyword = get_matching_keyword("string_literal", keywords);
                if (keyword != NULL) {
                    current_pair.keyword = *keyword;
                    push_token(syntaxTokens, current_pair);
                }
            } else if (prev_state == IN_SINGLELINE_COMMENT || prev_state == IN_MULTILINE_COMMENT) {
                keyword_t *keyword = get_matching_keyword("comment", keywords);
                if (keyword != NULL) {
                    current_pair.keyword = *keyword;
                    push_token(syntaxTokens, current_pair);
                }
            }
        }
        i += advance_count;
    }

    if (current_state == IN_TOKEN) {
        current_pair.end = i;
        buffer[buffer_len] = '\0';
        
        keyword_t *keyword = get_matching_keyword(buffer, keywords);
        if (keyword != NULL) {
            current_pair.keyword = *keyword;
            push_token(syntaxTokens, current_pair);
        }
    } else if (current_state == IN_MULTILINE_COMMENT) {
        current_pair.end = i;
        buffer[buffer_len] = '\0';

        keyword_t *keyword = get_matching_keyword("comment", keywords);
        if (keyword != NULL) {
            current_pair.keyword = *keyword;
            push_token(syntaxTokens, current_pair);
        }
    
    } else if (current_state == IN_SINGLELINE_COMMENT) {
        current_pair.end = i;
        buffer[buffer_len] = '\0';

        keyword_t *keyword = get_matching_keyword("comment", keywords);
        if (keyword != NULL) {
            current_pair.keyword = *keyword;
            push_token(syntaxTokens, current_pair);
        }
    }
}
