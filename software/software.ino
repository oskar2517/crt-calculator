#include <Keypad.h>
#include <ESP_8_BIT_GFX.h>

#define MAX_OUTPUT_SIZE (21 * 2)
#define MAX_INPUT_SIZE (21 * 3)

#define MAX_TOKENS 250

#define KEYPAD_ROWS 5
#define KEYPAD_COLS 4

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
  unsigned int token_count;
  Token *tokens;
} LexResult;

typedef struct {
  double value;
} ParseEvalResult;

char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'(',')','<','C'},
  {'7','8','9','/'},
  {'4','5','6','*'},
  {'1','2','3','-'},
  {'0','.','=','+'}
};

byte keypad_row_pins[KEYPAD_ROWS] = {13, 14, 15, 16, 17};
byte keypad_col_pins[KEYPAD_COLS] = {18, 19, 21, 22};

Keypad customKeypad = Keypad(makeKeymap(keypad_keys), keypad_row_pins, keypad_col_pins, KEYPAD_ROWS, KEYPAD_COLS);

ESP_8_BIT_GFX videoOut(false, 8 /* = RGB332 color */);

double lex_number(const char **input) {
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

LexResult *lex(const char *input) {
  LexResult *result = (LexResult*) malloc(sizeof(*result));
  if (!result) return NULL;

  Token *tokens = (Token*) malloc(MAX_TOKENS * sizeof(*tokens));
  if (!tokens) {
    free(result);

    return NULL;
  }

  result->tokens = tokens;

  unsigned int token_pointer = 0;
  const char *str = input;

  while (*str) {
    if (token_pointer >= MAX_TOKENS) {
      free(tokens);
      free(result);

      return NULL;
    }

    Token *t = &tokens[token_pointer];

    if (*str >= '0' && *str <= '9') {
      t->type = TOKEN_NUMBER;
      t->number = lex_number(&str);

      token_pointer++;
      continue;
    }

    switch (*str) {
      case '+': t->type = TOKEN_PLUS; break;
      case '-': t->type = TOKEN_MINUS; break;
      case '*': t->type = TOKEN_ASTERISK; break;
      case '/': t->type = TOKEN_SLASH; break;
      case '(': t->type = TOKEN_LEFT_PAREN; break;
      case ')': t->type = TOKEN_RIGHT_PAREN; break;
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

int8_t precedence(Token *token) {
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

bool parse_primary(LexResult *lex_result, unsigned int *position, double *value) {
  Token t = lex_result->tokens[*position];
  
  if (t.type == TOKEN_NUMBER) {
    (*position)++;
    *value = t.number;

    return true;
  }
  
  if (t.type == TOKEN_LEFT_PAREN) {
    (*position)++;
    ParseEvalResult *v = parse_eval_1(lex_result, position);
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
    case TOKEN_PLUS: return left + right;
    case TOKEN_MINUS: return left - right;
    case TOKEN_ASTERISK: return left * right;
    case TOKEN_SLASH: return left / right; // TODO: handle division by zero
  }

  // Unreachable
}

ParseEvalResult *parse_eval_2(LexResult *lex_result, unsigned int *position, double left, byte min_precedence) {
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

      ParseEvalResult *right_result =
        parse_eval_2(lex_result, position, right, precedence(&op) + 1);

      if (right_result == NULL) {
        return NULL;
      }

      right = right_result->value;
      free(right_result);
    }

    left = apply_operator(op.type, left, right);
  }

  ParseEvalResult *r = (ParseEvalResult*) malloc(sizeof(*r));
  if (!r) return NULL;

  r->value = left;
  return r;
}

ParseEvalResult *parse_eval_1(LexResult *lex_result, unsigned int *position) {
  double left = 0;
  
  if (!parse_primary(lex_result, position, &left)) {
    return NULL;
  }

  return parse_eval_2(lex_result, position, left, 0);
}

ParseEvalResult *parse_eval(LexResult *lex_result) {
  unsigned int position = 0;

  ParseEvalResult *result = parse_eval_1(lex_result, &position);

  if (position != lex_result->token_count) {
    return NULL;
  }

  return result;
}

void setup(void) {
  Serial.begin(9600);
  videoOut.begin();

  // Enable 12V boost converter
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
}
  
void loop(void) {
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