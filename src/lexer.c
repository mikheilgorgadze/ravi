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

void calculate_syntax_highlights(text_buffer_t *textBuffer, keyword_t *keywords, syntax_token_list_t *syntaxTokens){
    token_state_t state = START;

    char buffer[1024];
    syntax_token_t current_pair;
    size_t i = 0;
    int buffer_len = 0;
    syntaxTokens->count = 0;
    while (textBuffer->input[i] != '\0') {
        switch (state) {
            case START:
                if (isalnum(textBuffer->input[i]) || textBuffer->input[i] == '#') {
                    current_pair.start = i;
                    buffer_len = 0;
                    buffer[0] = textBuffer->input[i];
                    buffer_len ++;
                    state = IN_STRING;
                } else if (textBuffer->input[i] == '"') {
                    current_pair.start = i;
                    state = IN_DOUBLE_QUOTES;
                } else if (textBuffer->input[i] == '\'') {
                    current_pair.start = i;
                    state = IN_SINGLE_QUOTES;
                } else if (i < textBuffer->size && textBuffer->input[i] == '/' && textBuffer->input[i+1] == '/') {
                    current_pair.start = i;
                    state = IN_SINGLELINE_COMMENT;
                    i++;
                } else if (i < textBuffer->size && textBuffer->input[i] == '/' && textBuffer->input[i+1] == '*') {
                    current_pair.start = i;
                    state = IN_MULTILINE_COMMENT;
                    i++;
                } else if (isspace(textBuffer->input[i])) {
                    state = SPACE;
                } else {
                    state = START;
                }
            break;
            case IN_STRING:
                if (isalnum(textBuffer->input[i]) || textBuffer->input[i] == '#') {
                    state = IN_STRING;
                    if (buffer_len < 1023) {
                        buffer[buffer_len] = textBuffer->input[i];
                        buffer_len ++;
                    }
                } else if (textBuffer->input[i] == '"') {
                    state = IN_DOUBLE_QUOTES;
                } else if (textBuffer->input[i] == '\'') {
                    state = IN_SINGLE_QUOTES;
                } else {
                    current_pair.end = i;
                    buffer[buffer_len] = '\0';

                    keyword_t *keyword = get_matching_keyword(buffer, keywords);
                    if (keyword != NULL) {
                        current_pair.keyword = *keyword;
                        push_token(syntaxTokens, current_pair);
                    }

                    state = START; 
                    i--;
                }
            break;
            case IN_DOUBLE_QUOTES:
                if (textBuffer->input[i] == '"') {
                    int j = i - 1;
                    int backslashCount = 0;
                    while (j >= 0 && textBuffer->input[j] == '\\') {
                        backslashCount ++;
                        j--;
                    }
                    if (backslashCount % 2 != 0) {
                        state = IN_DOUBLE_QUOTES;
                        break;
                    }

                    current_pair.end = i + 1;
                    buffer[buffer_len] = '\0';

                    keyword_t *keyword = get_matching_keyword("string_literal", keywords);
                    if (keyword != NULL) {
                        current_pair.keyword = *keyword;
                        push_token(syntaxTokens, current_pair);
                    }

                    state = START;
                } else {
                    state = IN_DOUBLE_QUOTES;
                }
            break;
            case IN_SINGLE_QUOTES:
                if (textBuffer->input[i] == '\'') {
                    int j = i - 1;
                    int backslashCount = 0;
                    while (j >= 0 && textBuffer->input[j] == '\\') {
                        backslashCount ++;
                        j--;
                    }
                    if (backslashCount % 2 != 0) {
                        state = IN_SINGLE_QUOTES;
                        break;
                    }
                    current_pair.end = i + 1;
                    buffer[buffer_len] = '\0';

                    keyword_t *keyword = get_matching_keyword("single_quotes", keywords);
                    if (keyword != NULL) {
                        current_pair.keyword = *keyword;
                        push_token(syntaxTokens, current_pair);
                    }

                    state = START;
                } else {
                    state = IN_SINGLE_QUOTES;
                }
            break;
            case IN_SINGLELINE_COMMENT:
                if (textBuffer->input[i] == '\n') {
                    current_pair.end = i;
                    keyword_t *keyword = get_matching_keyword("comment", keywords);
                    if (keyword != NULL) {
                        current_pair.keyword = *keyword;
                        push_token(syntaxTokens, current_pair);
                    }
                    state = START;
                } else {
                    state = IN_SINGLELINE_COMMENT;
                }
            break;
            case IN_MULTILINE_COMMENT:
                if (i < textBuffer->size && textBuffer->input[i] == '*' && textBuffer->input[i+1] == '/') {
                    i++;
                    current_pair.end = i+1;
                    keyword_t *keyword = get_matching_keyword("comment", keywords);
                    if (keyword != NULL) {
                        current_pair.keyword = *keyword;
                        push_token(syntaxTokens, current_pair);
                    }
                    state = START;
                } else {
                    state = IN_MULTILINE_COMMENT;
                }

            break;
            case SPACE:
                if (isspace(textBuffer->input[i])) {
                    state = SPACE;
                } else {
                    state = START;
                    i--;
                }
            break;
        }
        i++;
    }

    if (state == IN_STRING) {
        current_pair.end = i;
        buffer[buffer_len] = '\0';
        
        keyword_t *keyword = get_matching_keyword(buffer, keywords);
        if (keyword != NULL) {
            current_pair.keyword = *keyword;
            push_token(syntaxTokens, current_pair);
        }
    } else if (state == IN_MULTILINE_COMMENT) {
        current_pair.end = i;
        buffer[buffer_len] = '\0';

        keyword_t *keyword = get_matching_keyword("comment", keywords);
        if (keyword != NULL) {
            current_pair.keyword = *keyword;
            push_token(syntaxTokens, current_pair);
        }
    
    } else if (state == IN_SINGLELINE_COMMENT) {
        current_pair.end = i;
        buffer[buffer_len] = '\0';

        keyword_t *keyword = get_matching_keyword("comment", keywords);
        if (keyword != NULL) {
            current_pair.keyword = *keyword;
            push_token(syntaxTokens, current_pair);
        }
    }
}
