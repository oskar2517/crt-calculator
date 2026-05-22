#pragma once

#include "lexer.h"

typedef struct {
    double value;
} ParseEvalResult;

ParseEvalResult* parse_eval(LexResult* lex_result);