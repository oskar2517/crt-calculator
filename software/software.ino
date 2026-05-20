#include <Keypad.h>
#include <ESP_8_BIT_GFX.h>

#define MAX_INPUT_LENGTH 512

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

void setup() {
  Serial.begin(9600);
  videoOut.begin();

  // Enable 12V boost converter
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
}

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
  
void loop(void) {
  static char input[MAX_INPUT_LENGTH + 1];
  static size_t input_pointer = 0;

  input[MAX_INPUT_LENGTH] = '\0';

  videoOut.waitForFrame();

  videoOut.fillScreen(0);
  videoOut.setCursor(0, 20);
  videoOut.setTextSize(2);
  videoOut.setTextWrap(true);
  videoOut.setTextColor(0xFF);
  videoOut.print(input);
 
  char read_key = customKeypad.getKey();

  if (!read_key) return;

  switch (read_key) {
    case 'C': {
      input_pointer = 0;
      memset(input, 0, sizeof(input));
      break;
    }

    case '=': {
      LexResult *lex_result = lex(input);

      if (lex_result == NULL) return;
      
      free(lex_result->tokens);
      free(lex_result);
      break;
    }

    default: {
      if (input_pointer >= MAX_INPUT_LENGTH) return;

      input[input_pointer++] = read_key;
      input[input_pointer] = '\0';
    }
  }
}