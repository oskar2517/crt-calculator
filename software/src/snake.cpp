#include "snake.h"

#include <Arduino.h>

#include "video.h"

#define TICK_SPEED 250

#define GRID_SIZE 10
#define FIELD_ROWS 18
#define FIELD_COLS 20

#define FIELD_X (GRID_SIZE * 2)
#define FIELD_Y (GRID_SIZE * 5)
#define FIELD_WIDTH (GRID_SIZE * FIELD_COLS)
#define FIELD_HEIGHT (GRID_SIZE * FIELD_ROWS)

#define MAX_SNAKE_LENGTH (FIELD_ROWS * FIELD_COLS)

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

typedef enum { GAME_OVER, PLAYING } GameState;

static Direction dir = RIGHT;
static GameState state = PLAYING;
static uint8_t snake[MAX_SNAKE_LENGTH][2];
static uint16_t fruit[2];
static uint16_t len = 1;
static uint32_t last_tick = 0;

static void reset_game();

static uint16_t ran_between(uint16_t min, uint16_t max) {
    return random(min, max + 1);
}

static void draw_snake() {
    for (uint16_t i = 0; i < len; i++) {
        video_out.drawRect(snake[i][0], snake[i][1], GRID_SIZE, GRID_SIZE,
                           0xFF);
    }
}

static void draw_fruit() {
    video_out.fillCircle(fruit[0] + GRID_SIZE / 2, fruit[1] + GRID_SIZE / 2,
                         GRID_SIZE / 2, 0xFF);
}

static void draw_field() {
    video_out.drawRect(FIELD_X, FIELD_Y, FIELD_WIDTH, FIELD_HEIGHT, 0xFF);
}

static void draw_score() {
    video_out.setCursor(FIELD_X, GRID_SIZE * 3);
    video_out.print("Score: ");
    video_out.print(len);
}

static void draw_game_over() {
    if (state != GAME_OVER) return;

    video_out.setCursor(FIELD_X + FIELD_WIDTH / 2 - 55,
                        FIELD_Y + 3 * GRID_SIZE);
    video_out.print("GAME OVER!");
}

static void update_fruit() {
    uint16_t ran_x =
        ran_between(FIELD_X + GRID_SIZE, FIELD_X + FIELD_WIDTH - GRID_SIZE);
    uint16_t ran_y =
        ran_between(FIELD_Y + GRID_SIZE, FIELD_Y + FIELD_HEIGHT - GRID_SIZE);

    fruit[0] = ran_x - (ran_x % GRID_SIZE);
    fruit[1] = ran_y - (ran_y % GRID_SIZE);
}

static void step_game() {
    if (state == GAME_OVER) return;

    // Update snake tail
    for (uint16_t i = len - 1; i > 0; i--) {
        snake[i][0] = snake[i - 1][0];
        snake[i][1] = snake[i - 1][1];
    }

    // Update snake head
    switch (dir) {
        case UP:
            snake[0][1] -= GRID_SIZE;
            break;
        case DOWN:
            snake[0][1] += GRID_SIZE;
            break;
        case LEFT:
            snake[0][0] -= GRID_SIZE;
            break;
        case RIGHT:
            snake[0][0] += GRID_SIZE;
            break;
    }

    uint16_t hx = snake[0][0];
    uint16_t hy = snake[0][1];

    // Check fruit collision
    if (hx == fruit[0] && hy == fruit[1]) {
        if (len < MAX_SNAKE_LENGTH) {
            len++;
            snake[len - 1][0] = -GRID_SIZE;
            snake[len - 1][1] = -GRID_SIZE;
        }
        update_fruit();
    }

    // Check lose condition: out of field bounds
    if (hx < FIELD_X || hx >= FIELD_X + FIELD_WIDTH || hy < FIELD_Y ||
        hy >= FIELD_Y + FIELD_HEIGHT) {
        state = GAME_OVER;
        return;
    }

    // Check lose condition: self collision
    for (uint16_t i = 1; i < len; i++) {
        if (snake[i][0] == snake[0][0] && snake[i][1] == snake[0][1]) {
            state = GAME_OVER;
            return;
        }
    }
}

void snake_handle_key(char read_key) {
    if (state == GAME_OVER) {
        reset_game();
    }

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

static void reset_game() {
    len = 1;
    dir = RIGHT;
    state = PLAYING;

    // Init snake head to center of field
    snake[0][0] = FIELD_X + (FIELD_WIDTH / 2 - ((FIELD_WIDTH / 2) % GRID_SIZE));
    snake[0][1] =
        FIELD_Y + (FIELD_HEIGHT / 2 - ((FIELD_HEIGHT / 2) % GRID_SIZE));

    update_fruit();
}

void snake_enter() {
    reset_game();
    last_tick = millis();
}

void snake_render() {
    video_out.waitForFrame();
    video_out.fillScreen(0);
    video_out.setTextSize(2);
    video_out.setTextColor(0xFF);

    if (millis() - last_tick >= TICK_SPEED) {
        last_tick = millis();

        step_game();
    }

    draw_score();
    draw_field();
    draw_fruit();
    draw_snake();
    draw_game_over();
}
