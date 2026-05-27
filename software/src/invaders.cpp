#include <stdint.h>

#include "keypad.h"
#include "video.h"

#define ENEMY_ROWS 4
#define ENEMY_COLS 7
#define ENEMY_SPACING 15
#define ENEMY_STEP_SIZE_X 2
#define ENEMY_STEP_SIZE_Y 10
#define ENEMY_COUNT (ENEMY_ROWS * ENEMY_COLS)

#define START_Y 10
#define END_X 122

typedef struct {
    const uint8_t* data;
    uint16_t width;
    uint16_t height;
} Sprite;

// TODO: unify into one type 'object'?
typedef struct {
    Sprite* sprite;
    int16_t x;
    int16_t y;
    bool alive;
} Enemy;

typedef struct {
    Sprite* sprite;
    int16_t x;
    int16_t y;
} Player;

// TODO: pack to integers
const uint8_t data_invader1[] = {
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
    0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
};

const uint8_t data_player[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

// TODO: allocate on heap?
Enemy enemies[ENEMY_COUNT];
uint32_t last_tick;
int8_t enemy_dir = 1;  // RIGHT
Player player;
int8_t player_dir = 1;  // RIGHT;

Sprite spr_enemy1 = {.data = data_invader1, .width = 11, .height = 8};

Sprite spr_player = {.data = data_player, .width = 13, .height = 8};

static void draw_sprite(Sprite* sprite, uint16_t x, uint16_t y, uint8_t scale) {
    for (uint16_t i = 0; i < sprite->width; i++) {
        for (uint16_t j = 0; j < sprite->height; j++) {
            video_out.fillRect((x + i) * scale, (y + j) * scale, scale, scale,
                               sprite->data[j * sprite->width + i]);
        }
    }
}

void invaders_enter() {
    for (uint8_t row = 0; row < ENEMY_ROWS; row++) {
        for (uint8_t col = 0; col < ENEMY_COLS; col++) {
            enemies[row * ENEMY_COLS + col] = {
                .sprite = &spr_enemy1,
                .x = (int16_t)(ENEMY_SPACING * col),
                .y = (int16_t)(START_Y + ENEMY_SPACING * row),
                .alive = true};
        }
    }

    player = {.sprite = &spr_player,
              .x = (int16_t)(END_X / 2 - spr_player.width / 2),
              .y = 100};

    last_tick = millis();
}

void update_enemies() {
    bool move_down = false;

    for (uint16_t i = 0; i < ENEMY_COUNT; i++) {
        Enemy* e = &enemies[i];

        e->x += ENEMY_STEP_SIZE_X * enemy_dir;

        if ((enemy_dir == 1 && e->x + e->sprite->width >= END_X) ||
            (enemy_dir == -1 && e->x <= 0)) {
            move_down = true;
        }
    }

    if (move_down) {
        enemy_dir = -enemy_dir;

        for (uint16_t i = 0; i < ENEMY_COUNT; i++) {
            Enemy* e = &enemies[i];
            e->y += ENEMY_STEP_SIZE_Y;
        }
    }
}

void update_player() {
    keypad.getKeys();

    bool left_down = false;
    bool right_down = false;

    for (uint8_t i = 0; i < LIST_MAX; i++) {
        if (keypad.key[i].kstate == PRESSED || keypad.key[i].kstate == HOLD) {
            if (keypad.key[i].kchar == '4') left_down = true;
            if (keypad.key[i].kchar == '6') right_down = true;
        }
    }

    if (left_down) {
        player.x -= 2;
    }

    if (right_down) {
        player.x += 2;
    }
}

void draw_enemies() {
    for (uint16_t i = 0; i < ENEMY_COUNT; i++) {
        Enemy* e = &enemies[i];
        draw_sprite(e->sprite, e->x, e->y, 2);
    }
}

void draw_player() { draw_sprite(player.sprite, player.x, player.y, 2); }

void invaders_exit() {}

void invaders_render() {
    video_out.waitForFrame();
    video_out.fillScreen(0);

    draw_player();

    update_player();

    if (millis() - last_tick > 500) {
        last_tick = millis();
        update_enemies();
    }
}

void invaders_handle_key(char read_key) {}