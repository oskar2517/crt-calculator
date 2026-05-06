#include <Keypad.h>
#include <ESP_8_BIT_GFX.h>

#define MAX_INPUT_LENGTH 128

#define KEYPAD_ROWS 5
#define KEYPAD_COLS 4

char keypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'(',')','<','C'},
  {'7','8','9','/'},
  {'4','5','6','*'},
  {'1','2','3','-'},
  {'0','.','=','+'}
};

byte keypadRowPins[KEYPAD_ROWS] = {13, 14, 15, 16, 17};
byte keypadColPins[KEYPAD_COLS] = {18, 19, 21, 22};

Keypad customKeypad = Keypad(makeKeymap(keypadKeys), keypadRowPins, keypadColPins, KEYPAD_ROWS, KEYPAD_COLS);

ESP_8_BIT_GFX videoOut(false, 8 /* = RGB332 color */);

struct ExpressionParser {
  const char *input;
  byte length;
  byte position;
  bool error;
};

bool isOperator(char key) {
  return key == '+' || key == '-' || key == '*' || key == '/';
}

int operatorPrecedence(char key) {
  if (key == '+' || key == '-') {
    return 1;
  }
  if (key == '*' || key == '/') {
    return 2;
  }
  return 0;
}

double parseNumber(ExpressionParser &parser) {
  double value = 0.0;
  double decimalPlace = 0.1;
  bool hasDigit = false;
  bool hasDecimalPoint = false;

  while (parser.position < parser.length) {
    char current = parser.input[parser.position];

    if (current >= '0' && current <= '9') {
      hasDigit = true;
      if (hasDecimalPoint) {
        value += (current - '0') * decimalPlace;
        decimalPlace *= 0.1;
      } else {
        value = value * 10.0 + (current - '0');
      }
      parser.position++;
    } else if (current == '.' && !hasDecimalPoint) {
      hasDecimalPoint = true;
      parser.position++;
    } else {
      break;
    }
  }

  if (!hasDigit) {
    parser.error = true;
  }

  return value;
}

double parsePrimary(ExpressionParser &parser) {
  if (parser.position >= parser.length) {
    parser.error = true;
    return 0.0;
  }

  char current = parser.input[parser.position];

  if (current == '-') {
    parser.position++;
    return -parsePrimary(parser);
  }

  if (current == '(') {
    parser.position++;
    double value = parseExpression(parser, 1);

    if (parser.position >= parser.length || parser.input[parser.position] != ')') {
      parser.error = true;
      return value;
    }

    parser.position++;
    return value;
  }

  if ((current >= '0' && current <= '9') || current == '.') {
    return parseNumber(parser);
  }

  parser.error = true;
  return 0.0;
}

double applyOperator(double left, char operatorKey, double right, ExpressionParser &parser) {
  if (operatorKey == '+') {
    return left + right;
  }
  if (operatorKey == '-') {
    return left - right;
  }
  if (operatorKey == '*') {
    return left * right;
  }
  if (operatorKey == '/') {
    if (right == 0.0) {
      parser.error = true;
      return 0.0;
    }
    return left / right;
  }

  parser.error = true;
  return 0.0;
}

double parseExpression(ExpressionParser &parser, int minimumPrecedence) {
  double left = parsePrimary(parser);

  while (!parser.error && parser.position < parser.length) {
    char operatorKey = parser.input[parser.position];
    int precedence = operatorPrecedence(operatorKey);

    if (precedence < minimumPrecedence) {
      break;
    }

    parser.position++;
    double right = parseExpression(parser, precedence + 1);
    left = applyOperator(left, operatorKey, right, parser);
  }

  return left;
}

bool evaluateExpression(const char input[], byte inputPointer, double &result) {
  if (inputPointer == 0) {
    return false;
  }

  ExpressionParser parser = { input, inputPointer, 0, false };
  result = parseExpression(parser, 1);

  return !parser.error && parser.position == parser.length;
}

bool isInput(char key) {
  return (key >= '0' && key <= '9') 
    || key == '(' || key == ')'
    || key == '+' || key == '-' || key == '*' || key == '/'
    || key == '.';
}

void setup() {
  Serial.begin(9600);
  videoOut.begin();

  // Enable 12V boost converter
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
}
  
void loop(void) {
  static char input[MAX_INPUT_LENGTH + 1];
  static byte inputPointer = 0;
  static bool hasResult = false;
  static double result;
  static char resultOutput[50];

  input[MAX_INPUT_LENGTH] = '\0';

  videoOut.waitForFrame();

  videoOut.fillScreen(0);
  videoOut.setCursor(0, 20);
  videoOut.setTextSize(2);
  videoOut.setTextWrap(true);
  videoOut.setTextColor(0xFF);
  videoOut.print(input);
 
  char readKey = customKeypad.getKey();
  
  if (isInput(readKey) && inputPointer < MAX_INPUT_LENGTH - 1) {
    if (hasResult) {
      inputPointer = 0;
      result = 0;
      hasResult = false;
      memset(input, 0, sizeof(input));
    }

    input[inputPointer] = readKey;
    inputPointer++;
  } else if (readKey == 'C') {
    inputPointer = 0;
    result = 0;
    memset(input, 0, sizeof(input));
    hasResult = false;
  } else if (readKey == '<' && inputPointer > 0) {
    inputPointer--;
  } else if (readKey == '=') {
    hasResult = true;

    if (evaluateExpression(input, inputPointer, result)) {
      Serial.println(result);
    } else {
      Serial.println("Error");
    }
  }

  if (hasResult) {
    memset(resultOutput, 0, sizeof(resultOutput));
    snprintf(resultOutput, 50, "%f", result);
    videoOut.print("\n= ");
    videoOut.print(resultOutput);
  }
}