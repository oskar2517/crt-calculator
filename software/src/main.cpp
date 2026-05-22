#include <Keypad.h>
#include <ESP_8_BIT_GFX.h>
#include "lexer.h"
#include "parser.h"
#include "keypad.h"
#include "video.h"

#define MAX_OUTPUT_SIZE (21 * 2)
#define MAX_INPUT_SIZE (21 * 3)

void setup() {
  Serial.begin(9600);
  videoOut.begin();

  // Enable 12V boost converter
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
}
  
void loop() {
  static char input[MAX_INPUT_SIZE + 1];
  static size_t input_pointer = 0;
  static char output[MAX_OUTPUT_SIZE];
  static bool has_output = false;
  static bool has_error = false; 
  static unsigned long last_cursor_blink = millis();
  static bool show_cursor = true;

  input[MAX_INPUT_SIZE] = '\0';

  videoOut.waitForFrame();

  videoOut.fillScreen(0);
  videoOut.setCursor(0, 20);
  videoOut.setTextSize(2);
  videoOut.setTextWrap(true);
  videoOut.setTextColor(0xFF);
  videoOut.print(input);

  if (!has_output) {
    if (show_cursor) {
      videoOut.print('_');
    }

    if (millis() - last_cursor_blink >= 500) {
      last_cursor_blink = millis();
      show_cursor = !show_cursor;
    }
  }

  if (has_output) {
    videoOut.print("\n= ");
    videoOut.print(output);
  }
 
  char read_key = customKeypad.getKey();

  if (!read_key) return;

  switch (read_key) {
    case 'C': {
      input_pointer = 0;
      has_output = false;
      has_error = false;
      memset(input, 0, sizeof(input));
      break;
    }

    case '=': {
      if (input_pointer == 0) return;

      LexResult *lex_result = lex(input);

      if (lex_result == NULL) {
        snprintf(output, MAX_OUTPUT_SIZE, "LEX ERROR");
        has_output = true;
        has_error = true;
        return;
      }

      ParseEvalResult *parse_eval_result = parse_eval(lex_result);

      free(lex_result->tokens);
      free(lex_result);

      if (parse_eval_result == NULL) {
        snprintf(output, MAX_OUTPUT_SIZE, "PARSE ERROR");
        has_output = true;
        has_error = true;
        return;
      }

      if (!isnormal(parse_eval_result->value)) {
        snprintf(output, MAX_OUTPUT_SIZE, "MATH ERROR");
        has_output = true;
        has_error = true;
        return;
      }

      memset(output, 0, sizeof(output));
      snprintf(output, MAX_OUTPUT_SIZE, "%f", parse_eval_result->value);
      has_output = true;

      free(parse_eval_result);

      break;
    }

    case '<': {
      if (has_output || input_pointer <= 0) return;

      // Do not blink while typing
      show_cursor = true;
      last_cursor_blink = millis();

      input_pointer--;
      input[input_pointer] = '\0';

      break;
    }

    default: {
      if (has_error) return;

      if (has_output) {
        has_output = false;
        memset(input, 0, sizeof(input));
        memcpy(input, output, sizeof(input));
        input_pointer = strlen(output);
      }

      if (input_pointer >= MAX_INPUT_SIZE) return;

      // Do not blink while typing
      show_cursor = true;
      last_cursor_blink = millis();

      input[input_pointer++] = read_key;
      input[input_pointer] = '\0';
    }
  }
}