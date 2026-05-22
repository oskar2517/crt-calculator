#include <Keypad.h>
#include <ESP_8_BIT_GFX.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "keypad.h"
#include "video.h"

#define MAX_OUTPUT_SIZE (21 * 2)
#define MAX_INPUT_SIZE (21 * 3)

typedef struct {
  char input[MAX_INPUT_SIZE + 1];
  size_t input_pointer;
  char output[MAX_OUTPUT_SIZE];
  bool has_output;
  bool has_error;
  unsigned long last_cursor_blink;
  bool show_cursor;
} AppState;

static AppState app_state = {{0}, 0, {0}, false, false, 0, true};

void reset_input(AppState* state) {
  state->input_pointer = 0;
  state->has_output = false;
  state->has_error = false;
  memset(state->input, 0, sizeof(state->input));
}

void reset_cursor_blink(AppState* state) {
  state->show_cursor = true;
  state->last_cursor_blink = millis();
}

void print_error(AppState* state, const char* message) {
  snprintf(state->output, sizeof(state->output), "%s", message);
  state->has_output = true;
  state->has_error = true;
}

void render_display(AppState* state) {
  state->input[MAX_INPUT_SIZE] = '\0';

  videoOut.waitForFrame();

  videoOut.fillScreen(0);
  videoOut.setCursor(0, 20);
  videoOut.setTextSize(2);
  videoOut.setTextWrap(true);
  videoOut.setTextColor(0xFF);
  videoOut.print(state->input);

  if (!state->has_output) {
    if (state->show_cursor) {
      videoOut.print('_');
    }

    if (millis() - state->last_cursor_blink >= 500) {
      state->last_cursor_blink = millis();
      state->show_cursor = !state->show_cursor;
    }
  }

  if (state->has_output) {
    videoOut.print("\n= ");
    videoOut.print(state->output);
  }
}

bool evaluate_input(AppState* state) {
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
  snprintf(state->output, sizeof(state->output), "%f", parse_eval_result->value);
  state->has_output = true;

  free(parse_eval_result);
  return true;
}

void delete_last_input_character(AppState* state) {
  if (state->has_output || state->input_pointer == 0) {
    return;
  }

  reset_cursor_blink(state);

  state->input_pointer--;
  state->input[state->input_pointer] = '\0';
}

void continue_from_output(AppState* state) {
  state->has_output = false;
  memset(state->input, 0, sizeof(state->input));
  snprintf(state->input, sizeof(state->input), "%s", state->output);
  state->input_pointer = strlen(state->input);
}

void append_input_character(AppState* state, char input_character) {
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

void handle_key(AppState* state, char read_key) {
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

void setup() {
  Serial.begin(9600);
  videoOut.begin();
  app_state.last_cursor_blink = millis();

  // Enable 12V boost converter
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
}
  
void loop() {
  render_display(&app_state);
 
  char read_key = customKeypad.getKey();

  if (!read_key) return;

  handle_key(&app_state, read_key);
}
