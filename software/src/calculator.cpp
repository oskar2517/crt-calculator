#include "calculator.h"

#include <Arduino.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "video.h"

#define MAX_OUTPUT_SIZE (21 * 2)
#define MAX_INPUT_SIZE (21 * 3)

typedef struct {
    char input[MAX_INPUT_SIZE + 1];
    uint8_t input_pointer;
    char output[MAX_OUTPUT_SIZE];
    bool has_output;
    bool has_error;
    uint32_t last_cursor_blink;
    bool show_cursor;
} CalculatorState;

static CalculatorState calculator_state = {{0}, 0, {0}, false, false, 0, true};

static void reset_input(CalculatorState* state) {
    state->input_pointer = 0;
    state->has_output = false;
    state->has_error = false;
    memset(state->input, 0, sizeof(state->input));
}

static void reset_cursor_blink(CalculatorState* state) {
    state->show_cursor = true;
    state->last_cursor_blink = millis();
}

static void print_error(CalculatorState* state, const char* message) {
    snprintf(state->output, sizeof(state->output), "%s", message);
    state->has_output = true;
    state->has_error = true;
}

static void render_display(CalculatorState* state) {
    state->input[MAX_INPUT_SIZE] = '\0';

    video_out.waitForFrame();

    video_out.fillScreen(0);
    video_out.setCursor(0, 20);
    video_out.setTextSize(2);
    video_out.setTextWrap(true);
    video_out.setTextColor(0xFF);
    video_out.print(state->input);

    if (!state->has_output) {
        if (state->show_cursor) {
            video_out.print('_');
        }

        if (millis() - state->last_cursor_blink >= 500) {
            state->last_cursor_blink = millis();
            state->show_cursor = !state->show_cursor;
        }
    }

    if (state->has_output) {
        video_out.print("\n= ");
        video_out.print(state->output);
    }
}

static bool evaluate_input(CalculatorState* state) {
    if (state->input_pointer == 0) {
        return false;
    }

    LexResult* lex_result = lex(state->input);

    if (lex_result == NULL) {
        print_error(state, "LEX ERROR");
        return false;
    }

    ParseEvalResult* parse_eval_result = parse_eval(lex_result);

    free(lex_result->tokens);
    free(lex_result);

    if (parse_eval_result == NULL) {
        print_error(state, "PARSE ERROR");
        return false;
    }

    if (!isnormal(parse_eval_result->value)) {
        print_error(state, "MATH ERROR");
        free(parse_eval_result);
        return false;
    }

    memset(state->output, 0, sizeof(state->output));
    snprintf(state->output, sizeof(state->output), "%f",
             parse_eval_result->value);
    state->has_output = true;

    free(parse_eval_result);
    return true;
}

static void delete_last_input_character(CalculatorState* state) {
    if (state->has_output || state->input_pointer == 0) {
        return;
    }

    reset_cursor_blink(state);

    state->input_pointer--;
    state->input[state->input_pointer] = '\0';
}

static void continue_from_output(CalculatorState* state) {
    state->has_output = false;
    memset(state->input, 0, sizeof(state->input));
    snprintf(state->input, sizeof(state->input), "%s", state->output);
    state->input_pointer = strlen(state->input);
}

static void append_input_character(CalculatorState* state,
                                   char input_character) {
    if (state->has_error) {
        return;
    }

    if (state->has_output) {
        continue_from_output(state);
    }

    if (state->input_pointer >= MAX_INPUT_SIZE) {
        return;
    }

    reset_cursor_blink(state);

    state->input[state->input_pointer++] = input_character;
    state->input[state->input_pointer] = '\0';
}

static void handle_key(CalculatorState* state, char read_key) {
    switch (read_key) {
        case 'C':
            reset_input(state);
            break;

        case '=':
            evaluate_input(state);
            break;

        case '<':
            delete_last_input_character(state);
            break;

        default:
            append_input_character(state, read_key);
            break;
    }
}

void calculator_enter() { reset_cursor_blink(&calculator_state); }

void calculator_render() { render_display(&calculator_state); }

void calculator_handle_key(char read_key) {
    handle_key(&calculator_state, read_key);
}
