#pragma once

#include <stdint.h>

#define MAX_TOKENS 250

typedef enum {
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN
} TokenType;

typedef struct {
    TokenType type;

    union {
        double number;
    };
} Token;

typedef struct {
    uint32_t token_count;
    Token* tokens;
} LexResult;

LexResult* lex(const char* input);
