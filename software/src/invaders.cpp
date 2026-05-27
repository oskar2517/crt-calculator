// TODO: better error handling for allocation failures

#include <stdint.h>
#include <stdlib.h>

#include "keypad.h"
#include "video.h"

#define ENEMY_ROWS 4
#define ENEMY_COLS 7
#define ENEMY_SPACING 15
#define ENEMY_STEP_SIZE_X 2
#define ENEMY_STEP_SIZE_Y 5
#define ENEMY_COUNT (ENEMY_ROWS * ENEMY_COLS)

#define PLAYER_STEP_SIZE 2

#define BULLET_STEP_SIZE 2
#define MAX_BULLET_COUNT 40

#define START_Y 10
#define END_X 122

typedef struct {
    const uint8_t* data;
    uint16_t width;
    uint16_t height;
} Sprite;

// TODO: unify into one type 'entity'?
typedef struct {
    Sprite* sprite;
    int16_t x;
    int16_t y;
} Enemy;

typedef struct {
    Sprite* sprite;
    int16_t x;
    int16_t y;
} Player;

typedef struct {
    Sprite* sprite;
    int16_t x;
    int16_t y;
    int8_t dir;
} Bullet;

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

const uint8_t data_bullet[] = {
    0xFF,
    0xFF,
    0xFF,
    0xFF,
};

uint32_t last_tick;

Enemy** enemies;
int8_t enemy_dir;  // RIGHT

Player player;
int8_t player_dir;  // RIGHT;

Bullet** bullets;

Sprite spr_enemy1 = {.data = data_invader1, .width = 11, .height = 8};

Sprite spr_player = {.data = data_player, .width = 13, .height = 8};

Sprite spr_bullet = {.data = data_bullet, .width = 1, .height = 4};

static void draw_sprite(Sprite* sprite, uint16_t x, uint16_t y, uint8_t scale) {
    for (uint16_t i = 0; i < sprite->width; i++) {
        for (uint16_t j = 0; j < sprite->height; j++) {
            video_out.fillRect((x + i) * scale, (y + j) * scale, scale, scale,
                               sprite->data[j * sprite->width + i]);
        }
    }
}

void invaders_enter() {
    enemy_dir = 1;   // RIGHT
    player_dir = 1;  // RIGHT

    bullets = (Bullet**)calloc(MAX_BULLET_COUNT, sizeof(Bullet*));
    if (bullets == NULL) return;

    enemies = (Enemy**)malloc(ENEMY_COUNT * sizeof(Enemy*));
    if (enemies == NULL) return;

    for (uint8_t row = 0; row < ENEMY_ROWS; row++) {
        for (uint8_t col = 0; col < ENEMY_COLS; col++) {
            Enemy* e = (Enemy*)malloc(sizeof(Enemy));
            if (e == NULL) return;

            e->sprite = &spr_enemy1;
            e->x = (int16_t)(ENEMY_SPACING * col);
            e->y = (int16_t)(START_Y + ENEMY_SPACING * row);

            enemies[row * ENEMY_COLS + col] = e;
        }
    }

    player = {.sprite = &spr_player,
              .x = (int16_t)(END_X / 2 - spr_player.width / 2),
              .y = 100};

    last_tick = millis();
}

void update_enemies() {
    bool move_down = false;

    // Move enemies - x
    for (uint16_t i = 0; i < ENEMY_COUNT; i++) {
        Enemy* e = enemies[i];

        e->x += ENEMY_STEP_SIZE_X * enemy_dir;

        if ((enemy_dir == 1 && e->x + e->sprite->width >= END_X) ||
            (enemy_dir == -1 && e->x <= 0)) {
            move_down = true;
        }
    }

    // Move enemies - y
    if (move_down) {
        enemy_dir = -enemy_dir;

        for (uint16_t i = 0; i < ENEMY_COUNT; i++) {
            enemies[i]->y += ENEMY_STEP_SIZE_Y;
        }
    }
}

void update_bullets() {
    for (uint8_t i = 0; i < MAX_BULLET_COUNT; i++) {
        Bullet* b = bullets[i];

        if (b == NULL) continue;

        b->y += BULLET_STEP_SIZE * b->dir;

        if (b->y + b->sprite->height < 0 ||
            b->y > player.y + player.sprite->height) {
            free(b);
            bullets[i] = NULL;
        }
    }
}

void create_bullet(uint16_t x, uint16_t y, int8_t dir) {
    int8_t free_index = -1;
    for (uint8_t i = 0; i < MAX_BULLET_COUNT; i++) {
        if (bullets[i] == NULL) {
            free_index = i;
            break;
        }
    }

    if (free_index == -1) return;

    Bullet* b = (Bullet*)malloc(sizeof(Bullet));
    if (b == NULL) return;

    b->sprite = &spr_bullet;
    b->x = x;
    b->y = y;
    b->dir = dir;

    bullets[free_index] = b;
}

void update_player() {
    static uint32_t last_shot = millis();

    keypad.getKeys();

    bool move_left = false;
    bool move_right = false;

    for (uint8_t i = 0; i < LIST_MAX; i++) {
        if (keypad.key[i].kstate == PRESSED || keypad.key[i].kstate == HOLD) {
            switch (keypad.key[i].kchar) {
                case '4':
                    move_left = true;
                    break;
                case '6':
                    move_right = true;
                    break;
                case '8':
                    if (millis() - last_shot > 200) {
                        last_shot = millis();
                        create_bullet(player.x + player.sprite->width / 2,
                                      player.y - spr_bullet.height, -1);
                    }
                    break;
            }
        }
    }

    if (move_left && player.x > 0) {
        player.x -= PLAYER_STEP_SIZE;
    }

    if (move_right && player.x + player.sprite->width < END_X) {
        player.x += PLAYER_STEP_SIZE;
    }
}

void draw_enemies() {
    for (uint16_t i = 0; i < ENEMY_COUNT; i++) {
        Enemy* e = enemies[i];
        draw_sprite(e->sprite, e->x, e->y, 2);
    }
}

void draw_bullets() {
    for (uint8_t i = 0; i < MAX_BULLET_COUNT; i++) {
        Bullet* b = bullets[i];
        if (b == NULL) continue;
        if (b->y < 0) continue;

        draw_sprite(b->sprite, b->x, b->y, 2);
    }
}

void draw_player() { draw_sprite(player.sprite, player.x, player.y, 2); }

void invaders_exit() {
    for (uint8_t i = 0; i < ENEMY_COUNT; i++) {
        if (enemies[i] != NULL) free(enemies[i]);
    }
    free(enemies);

    for (uint8_t i = 0; i < MAX_BULLET_COUNT; i++) {
        if (bullets[i] != NULL) free(bullets[i]);
    }
    free(bullets);
}

void invaders_render() {
    video_out.waitForFrame();
    video_out.fillScreen(0);

    update_player();
    update_bullets();

    if (millis() - last_tick > 500) {
        last_tick = millis();
        update_enemies();
    }

    draw_enemies();
    draw_player();
    draw_bullets();
}

void invaders_handle_key(char read_key) {}
