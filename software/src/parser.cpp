#include "parser.h"

#include <stdint.h>
#include <stdlib.h>

#include "lexer.h"

ParseEvalResult* parse_eval_1(LexResult* lex_result, unsigned int* position);

int8_t precedence(Token* token) {
    switch (token->type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 1;

        case TOKEN_ASTERISK:
        case TOKEN_SLASH:
            return 2;
    }

    return -1;
}

bool parse_primary(LexResult* lex_result, uint32_t* position, double* value) {
    Token t = lex_result->tokens[*position];

    if (t.type == TOKEN_NUMBER) {
        (*position)++;
        *value = t.number;

        return true;
    }

    if (t.type == TOKEN_LEFT_PAREN) {
        (*position)++;

        ParseEvalResult* v = parse_eval_1(lex_result, position);
        if (v == NULL) {
            return false;
        }

        if (lex_result->tokens[*position].type != TOKEN_RIGHT_PAREN) {
            return false;
        }

        (*position)++;

        *value = v->value;
        free(v);

        return true;
    }

    if (t.type == TOKEN_MINUS) {
        (*position)++;

        double inner = 0;
        if (!parse_primary(lex_result, position, &inner)) {
            return false;
        }

        *value = -inner;
        return true;
    }

    return false;
}

double apply_operator(TokenType op, double left, double right) {
    switch (op) {
        case TOKEN_PLUS:
            return left + right;
        case TOKEN_MINUS:
            return left - right;
        case TOKEN_ASTERISK:
            return left * right;
        case TOKEN_SLASH:
            return left / right;
    }

    // Unreachable
}

ParseEvalResult* parse_eval_2(LexResult* lex_result, uint32_t* position,
                              double left, int8_t min_precedence) {
    while (*position < lex_result->token_count) {
        Token lookahead = lex_result->tokens[*position];

        if (precedence(&lookahead) < min_precedence) {
            break;
        }

        Token op = lookahead;
        (*position)++;

        double right = 0;
        if (!parse_primary(lex_result, position, &right)) {
            return NULL;
        }

        while (*position < lex_result->token_count) {
            lookahead = lex_result->tokens[*position];

            if (precedence(&lookahead) <= precedence(&op)) {
                break;
            }

            ParseEvalResult* right_result =
                parse_eval_2(lex_result, position, right, precedence(&op) + 1);

            if (right_result == NULL) {
                return NULL;
            }

            right = right_result->value;
            free(right_result);
        }

        left = apply_operator(op.type, left, right);
    }

    ParseEvalResult* r = (ParseEvalResult*)malloc(sizeof(*r));
    if (!r) return NULL;

    r->value = left;
    return r;
}

ParseEvalResult* parse_eval_1(LexResult* lex_result, uint32_t* position) {
    double left = 0;

    if (!parse_primary(lex_result, position, &left)) {
        return NULL;
    }

    return parse_eval_2(lex_result, position, left, 0);
}

ParseEvalResult* parse_eval(LexResult* lex_result) {
    uint32_t position = 0;

    ParseEvalResult* result = parse_eval_1(lex_result, &position);

    if (position != lex_result->token_count) {
        return NULL;
    }

    return result;
}