#include <ESP_8_BIT_GFX.h>
#include <Keypad.h>
#include <random>

#define KEYPAD_ROWS 5
#define KEYPAD_COLS 4

#define MAX_SNAKE_LENGTH 256 // TODO: use max length that fits field
#define TICK_SPEED 250

#define GRID_SIZE 10
#define FIELD_X GRID_SIZE * 2
#define FIELD_Y GRID_SIZE * 5
#define FIELD_WIDTH GRID_SIZE * 20
#define FIELD_HEIGHT GRID_SIZE * 18

typedef enum {
  UP,
  DOWN,
  LEFT,
  RIGHT
} Direction;

char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'(',')','<','C'},
  {'7','8','9','/'},
  {'4','5','6','*'},
  {'1','2','3','-'},
  {'0','.','=','+'}
};

uint8_t keypad_row_pins[KEYPAD_ROWS] = {13, 14, 15, 16, 17};
uint8_t keypad_col_pins[KEYPAD_COLS] = {18, 19, 21, 22};

Keypad customKeypad = Keypad(makeKeymap(keypad_keys), keypad_row_pins, keypad_col_pins, KEYPAD_ROWS, KEYPAD_COLS);

ESP_8_BIT_GFX videoOut(false /* = PAL */, 8 /* = RGB332 color */);

std::random_device rd;
std::mt19937 rng(rd());

Direction dir = RIGHT;
uint8_t snake[MAX_SNAKE_LENGTH][2];
uint16_t fruit[2];
uint16_t len = 1;

uint16_t ran_between(uint16_t min, uint16_t max) {
    std::uniform_int_distribution<uint16_t> dist(min, max);
    return dist(rng);
}

void draw_snake() {
  for (uint8_t i = 0; i < len; i++) {
    videoOut.drawRect(snake[i][0], snake[i][1], GRID_SIZE, GRID_SIZE, 0xFF);
  }
}

void draw_fruit() {
  videoOut.fillCircle(fruit[0] + GRID_SIZE / 2, fruit[1] + GRID_SIZE / 2, GRID_SIZE / 2, 0xFF);
}

void draw_field() {
  videoOut.drawRect(FIELD_X, FIELD_Y, FIELD_WIDTH, FIELD_HEIGHT, 0xFF);
}

void draw_score() {
  videoOut.setCursor(FIELD_X, GRID_SIZE * 3);
  videoOut.setTextSize(2);
  videoOut.setTextColor(0xFF);
  videoOut.print("Score: ");
  videoOut.print(len);
}

void update_fruit() {
  uint16_t ran_x = ran_between(FIELD_X + GRID_SIZE, FIELD_X + FIELD_WIDTH - GRID_SIZE);
  uint16_t ran_y = ran_between(FIELD_Y + GRID_SIZE, FIELD_Y + FIELD_HEIGHT - GRID_SIZE);

  fruit[0] = ran_x - (ran_x % GRID_SIZE);
  fruit[1] = ran_y - (ran_y % GRID_SIZE);
}

void step_game() {
  for (uint8_t i = len - 1; i > 0; i--) {
    snake[i][0] = snake[i - 1][0];
    snake[i][1] = snake[i - 1][1];
  }
  
  switch (dir) {
    case UP: snake[0][1] -= GRID_SIZE; break;
    case DOWN: snake[0][1] += GRID_SIZE; break;
    case LEFT: snake[0][0] -= GRID_SIZE; break;
    case RIGHT: snake[0][0] += GRID_SIZE; break;
  }

  uint16_t hx = snake[0][0];
  uint16_t hy = snake[0][1];
  
  if (hx == fruit[0] && hy == fruit[1]) {
    len++;
    snake[len - 1][0] = -GRID_SIZE;
    snake[len - 1][1] = -GRID_SIZE;
    update_fruit();
  }

  // Check lose condition: out of field bounds
  if (hx < FIELD_X || hx >= FIELD_X + FIELD_WIDTH || hy < FIELD_Y || hy >= FIELD_Y + FIELD_HEIGHT) {
    reset_game();
  }

  // Check lose condition: self collision
  for (uint16_t i = 1; i < len; i++) {
    if (snake[i][0] == snake[0][0] && snake[i][1] == snake[0][1]) {
      reset_game();
    }
  }
}

void handle_input() {
  char read_key = customKeypad.getKey();

  if (read_key) {
    switch (read_key) {
      case '8': 
        if (dir == DOWN) break;
        dir = UP; 
        break;

      case '2':
        if (dir == UP) break;
        dir = DOWN; 
        break;

      case '4': 
        if (dir == RIGHT) break;
        dir = LEFT; 
        break;

      case '6':
        if (dir == LEFT) break;
        dir = RIGHT; 
        break;
    }
  }
}

void reset_game() {
  len = 1;
  dir = RIGHT;

  // Init snake head
  snake[0][0] = FIELD_X + (FIELD_WIDTH / 2 - ((FIELD_WIDTH / 2) % GRID_SIZE));
  snake[0][1] = FIELD_Y + (FIELD_HEIGHT / 2 - ((FIELD_HEIGHT / 2) % GRID_SIZE));

  update_fruit(); 
}

void setup() {
  Serial.begin(9600);
  videoOut.begin();

  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);

  reset_game();
}

void loop() {
  static uint32_t last_tick = millis();

  videoOut.waitForFrame();
  videoOut.fillScreen(0);

  handle_input();

  if (millis() - last_tick >= TICK_SPEED) {
    last_tick = millis();

    /* Serial.print("Head: ");
    Serial.print(snake[0][0]);
    Serial.print("/");
    Serial.print(snake[0][1]);

    Serial.print(" Fruit: ");
    Serial.print(fruit[0]);
    Serial.print("/");
    Serial.print(fruit[1]);

    Serial.print(" Len: ");
    Serial.println(len); */

    step_game();
  }

  draw_score();
  draw_field();
  draw_fruit();
  draw_snake();
}