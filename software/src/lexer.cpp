#include "lexer.h"

#include <stdint.h>
#include <stdlib.h>

double lex_number(const char** input) {
    double value = 0.0;
    double divisor = 10.0;

    while (**input >= '0' && **input <= '9') {
        value = (value * 10) + (**input - '0');
        (*input)++;
    }

    if (**input == '.') {
        (*input)++;

        while (**input >= '0' && **input <= '9') {
            value += (**input - '0') / divisor;
            divisor *= 10.0;
            (*input)++;
        }
    }

    return value;
}

LexResult* lex(const char* input) {
    LexResult* result = (LexResult*)malloc(sizeof(*result));
    if (!result) return NULL;

    Token* tokens = (Token*)malloc(MAX_TOKENS * sizeof(*tokens));
    if (!tokens) {
        free(result);

        return NULL;
    }

    result->tokens = tokens;

    uint32_t token_pointer = 0;
    const char* str = input;

    while (*str) {
        if (token_pointer >= MAX_TOKENS) {
            free(tokens);
            free(result);

            return NULL;
        }

        Token* t = &tokens[token_pointer];

        if (*str >= '0' && *str <= '9') {
            t->type = TOKEN_NUMBER;
            t->number = lex_number(&str);

            token_pointer++;
            continue;
        }

        switch (*str) {
            case '+':
                t->type = TOKEN_PLUS;
                break;
            case '-':
                t->type = TOKEN_MINUS;
                break;
            case '*':
                t->type = TOKEN_ASTERISK;
                break;
            case '/':
                t->type = TOKEN_SLASH;
                break;
            case '(':
                t->type = TOKEN_LEFT_PAREN;
                break;
            case ')':
                t->type = TOKEN_RIGHT_PAREN;
                break;
            default:
                free(tokens);
                free(result);

                return NULL;
        }

        token_pointer++;
        str++;
    }

    result->token_count = token_pointer;

    return result;
}
