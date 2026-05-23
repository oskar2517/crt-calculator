#include "snake.h"

#include <Arduino.h>

#include "video.h"

#define BASE_TICK_SPEED 1000

#define GRID_SIZE 10
#define FIELD_ROWS 18
#define FIELD_COLS 20

#define FIELD_X (GRID_SIZE * 2)
#define FIELD_Y (GRID_SIZE * 5)
#define FIELD_WIDTH (GRID_SIZE * FIELD_COLS)
#define FIELD_HEIGHT (GRID_SIZE * FIELD_ROWS)

#define MAX_SNAKE_LENGTH (FIELD_ROWS * FIELD_COLS)

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;

typedef enum { GS_MENU, GS_PLAYING, GS_GAME_OVER } GameState;

static uint8_t difficulty = 4;
static Direction dir = DIR_RIGHT;
static GameState state = GS_MENU;
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
    video_out.setCursor(FIELD_X + FIELD_WIDTH / 2 - 55,
                        FIELD_Y + 3 * GRID_SIZE);
    video_out.print("GAME OVER!");
}

static void draw_menu() {
    video_out.setCursor(0, GRID_SIZE * 3);
    video_out.setTextSize(4);
    video_out.println("SNAKE!");
    video_out.setTextSize(2);
    video_out.println();

    video_out.println("Select Difficulty:\n");
    video_out.println("1 = Very Easy");
    video_out.println("2 = Easy");
    video_out.println("3 = Normal");
    video_out.println("4 = Hard");
    video_out.println("5 = Very Hard");
    video_out.println("6 = Extremely Hard");
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
    // Update snake tail
    for (uint16_t i = len - 1; i > 0; i--) {
        snake[i][0] = snake[i - 1][0];
        snake[i][1] = snake[i - 1][1];
    }

    // Update snake head
    switch (dir) {
        case DIR_UP:
            snake[0][1] -= GRID_SIZE;
            break;
        case DIR_DOWN:
            snake[0][1] += GRID_SIZE;
            break;
        case DIR_LEFT:
            snake[0][0] -= GRID_SIZE;
            break;
        case DIR_RIGHT:
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
        state = GS_GAME_OVER;
        return;
    }

    // Check lose condition: self collision
    for (uint16_t i = 1; i < len; i++) {
        if (snake[i][0] == snake[0][0] && snake[i][1] == snake[0][1]) {
            state = GS_GAME_OVER;
            return;
        }
    }
}

void snake_handle_key(char read_key) {
    if (state == GS_MENU) {
        if (read_key < '1' && read_key > '6') return;

        difficulty = (read_key - 0x30 - 1) * 4;
        if (difficulty == 0) difficulty = 2;

        state = GS_PLAYING;

        return;
    }

    if (state == GS_GAME_OVER) {
        reset_game();
        return;
    }

    switch (read_key) {
        case '8':
            if (dir == DIR_DOWN) break;
            dir = DIR_UP;
            break;

        case '2':
            if (dir == DIR_UP) break;
            dir = DIR_DOWN;
            break;

        case '4':
            if (dir == DIR_RIGHT) break;
            dir = DIR_LEFT;
            break;

        case '6':
            if (dir == DIR_LEFT) break;
            dir = DIR_RIGHT;
            break;
    }
}

static void reset_game() {
    len = 1;
    dir = DIR_RIGHT;
    state = GS_MENU;

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

void snake_exit() {}

void snake_render() {
    video_out.waitForFrame();
    video_out.fillScreen(0);
    video_out.setTextSize(2);
    video_out.setTextColor(0xFF);

    switch (state) {
        case GS_GAME_OVER:
            draw_game_over();

        case GS_PLAYING:
            if (state == GS_PLAYING &&
                millis() - last_tick >= (BASE_TICK_SPEED / difficulty)) {
                last_tick = millis();

                step_game();
            }

            draw_score();
            draw_field();
            draw_fruit();
            draw_snake();
            break;

        case GS_MENU:
            draw_menu();
            break;
    }
}
