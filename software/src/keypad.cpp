#include <Keypad.h>
#include <stdint.h>

#define KEYPAD_ROWS 5
#define KEYPAD_COLS 4

char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] = {{'(', ')', '<', 'C'},
                                              {'7', '8', '9', '/'},
                                              {'4', '5', '6', '*'},
                                              {'1', '2', '3', '-'},
                                              {'0', '.', '=', '+'}};

uint8_t keypad_row_pins[KEYPAD_ROWS] = {13, 14, 15, 16, 17};
uint8_t keypad_col_pins[KEYPAD_COLS] = {18, 19, 21, 22};

Keypad keypad = Keypad(makeKeymap(keypad_keys), keypad_row_pins,
                             keypad_col_pins, KEYPAD_ROWS, KEYPAD_COLS);