// TODO: better error handling for allocation failures

#include <stdint.h>
#include <stdlib.h>

#include "keypad.h"
#include "video.h"

#define ENEMY_VALUE 10
#define ENEMY_ROWS 3
#define ENEMY_COLS 7
#define ENEMY_SPACING 30
#define ENEMY_STEP_SIZE_X 4
#define ENEMY_STEP_SIZE_Y 8
#define ENEMY_COUNT (ENEMY_ROWS * ENEMY_COLS)

#define PLAYER_STEP_SIZE 4

#define BULLET_STEP_SIZE 4
#define MAX_BULLET_COUNT 40

#define BARRICADES_Y 185
#define BARRICADE_GROUPS 4
#define BARRICADE_COLS 12
#define BARRICADE_ROWS 8
#define BARRICADE_BLOCK_STEP 2
#define BARRICADE_BLOCKS_PER_GROUP (BARRICADE_COLS * BARRICADE_ROWS)
#define BARRICADES_COUNT (BARRICADE_GROUPS * BARRICADE_COLS * BARRICADE_ROWS)

#define EXPLOSION_COLS 5
#define EXPLOSION_ROWS 5

#define ENTITY_SCALE 2

#define START_Y 50
#define START_X 2
#define END_X 244

typedef enum { GS_MENU, GS_PLAYING, GS_GAME_OVER, GS_PAUSED } GameState;

typedef struct {
    const uint8_t* data;
    uint16_t width;
    uint16_t height;
} Sprite;

typedef enum {
    ENTITY_ENEMY,
    ENTITY_PLAYER,
    ENTITY_BULLET,
    ENTITY_BARRICADE
} EntityType;

typedef struct {
    EntityType type;
    Sprite* sprite;
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;
} Entity;

// TODO: pack to integers
static const uint8_t data_enemy1[] = {
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
    0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
};

static const uint8_t data_enemy2[] = {
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
    0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
};

static const uint8_t data_player[] = {
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

static const uint8_t data_bullet[] = {
    0xFF,
    0xFF,
    0xFF,
    0xFF,
};

static const uint8_t data_explosion[] = {
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
    0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
    0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00};

static const uint8_t data_barricade_block[] = {0xFF};

static const uint8_t mask_barricade_shape[BARRICADE_ROWS][BARRICADE_COLS] = {
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1}, {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
};

static const uint8_t mask_explosion_shape[EXPLOSION_ROWS][EXPLOSION_COLS] = {
    {0, 0, 1, 0, 0},
    {0, 1, 1, 1, 0},
    {1, 1, 1, 1, 1},
    {0, 1, 1, 1, 0},
    {0, 0, 1, 0, 0}};

static Sprite spr_enemy1 = {.data = data_enemy1, .width = 11, .height = 8};

static Sprite spr_enemy2 = {.data = data_enemy2, .width = 11, .height = 8};

static Sprite spr_player = {.data = data_player, .width = 13, .height = 8};

static Sprite spr_bullet = {.data = data_bullet, .width = 1, .height = 4};

static Sprite spr_explosion = {
    .data = data_explosion, .width = 13, .height = 8};

static Sprite spr_barricade_block = {
    .data = data_barricade_block, .width = 1, .height = 1};

static GameState state;
static uint16_t score;

static Entity** enemies = NULL;
static uint8_t living_enemies_count;

static int8_t pending_enemy_removal_index = -1;
static uint32_t last_enemy_update_tick = 0;
static uint32_t last_enemy_shoot_tick = 0;
static uint32_t last_player_shot_tick = 0;
static int64_t paused_tick = -1;

static Entity player;

static Entity** bullets = NULL;

static Entity** barricades = NULL;

static void draw_game_over() {
    video_out.setCursor(130, 20);
    video_out.print("GAME OVER!");
}

static void draw_sprite(Sprite* sprite, int16_t x, int16_t y, uint8_t scale) {
    for (uint16_t i = 0; i < sprite->width; i++) {
        for (uint16_t j = 0; j < sprite->height; j++) {
            video_out.fillRect(x + i * scale, y + j * scale, scale, scale,
                               sprite->data[j * sprite->width + i]);
        }
    }
}

static int16_t scaled_sprite_width(Sprite* sprite) {
    return (int16_t)(sprite->width * ENTITY_SCALE);
}

static int16_t scaled_sprite_height(Sprite* sprite) {
    return (int16_t)(sprite->height * ENTITY_SCALE);
}

static int16_t entity_width(Entity* entity) {
    return scaled_sprite_width(entity->sprite);
}

static int16_t entity_height(Entity* entity) {
    return scaled_sprite_height(entity->sprite);
}

static void reset_timers() {
    uint32_t now = millis();
    last_enemy_update_tick = now;
    last_enemy_shoot_tick = now;
    last_player_shot_tick = now;
    paused_tick = -1;
}

static void free_resources() {
    if (enemies != NULL) {
        for (uint8_t i = 0; i < ENEMY_COUNT; i++) {
            if (enemies[i] != NULL) free(enemies[i]);
        }
        free(enemies);
        enemies = NULL;
    }

    if (bullets != NULL) {
        for (uint8_t i = 0; i < MAX_BULLET_COUNT; i++) {
            if (bullets[i] != NULL) free(bullets[i]);
        }
        free(bullets);
        bullets = NULL;
    }

    if (barricades != NULL) {
        for (uint16_t i = 0; i < BARRICADES_COUNT; i++) {
            if (barricades[i] != NULL) free(barricades[i]);
        }
        free(barricades);
        barricades = NULL;
    }

    pending_enemy_removal_index = -1;
    reset_timers();
}

static Entity* allocate_barricade_block(int16_t x, int16_t y) {
    Entity* b = (Entity*)malloc(sizeof(Entity));
    if (b == NULL) return NULL;

    b->sprite = &spr_barricade_block;
    b->type = ENTITY_BARRICADE;
    b->x = x;
    b->y = y;
    b->dx = 0;
    b->dy = 0;

    return b;
}

static uint16_t barricade_index(uint8_t group, uint8_t x, uint8_t y) {
    return (uint16_t)(group * BARRICADE_BLOCKS_PER_GROUP + y * BARRICADE_COLS +
                      x);
}

static void create_barricades() {
    int16_t barricade_width = BARRICADE_COLS * BARRICADE_BLOCK_STEP;
    int16_t barricade_spacing =
        (END_X - BARRICADE_GROUPS * barricade_width) / (BARRICADE_GROUPS + 1);

    for (uint8_t group = 0; group < BARRICADE_GROUPS; group++) {
        int16_t barricade_x =
            barricade_spacing + group * (barricade_width + barricade_spacing);

        for (uint8_t y = 0; y < BARRICADE_ROWS; y++) {
            for (uint8_t x = 0; x < BARRICADE_COLS; x++) {
                if (mask_barricade_shape[y][x] == 0) continue;

                Entity* block = allocate_barricade_block(
                    barricade_x + x * BARRICADE_BLOCK_STEP,
                    BARRICADES_Y + y * BARRICADE_BLOCK_STEP);
                if (block == NULL) return;

                barricades[barricade_index(group, x, y)] = block;
            }
        }
    }
}

static void reset_board() {
    free_resources();

    bullets = (Entity**)calloc(MAX_BULLET_COUNT, sizeof(Entity*));
    if (bullets == NULL) return;

    living_enemies_count = ENEMY_COUNT;
    enemies = (Entity**)calloc(ENEMY_COUNT, sizeof(Entity*));
    if (enemies == NULL) return;

    barricades = (Entity**)calloc(BARRICADES_COUNT, sizeof(Entity*));
    if (barricades == NULL) return;

    create_barricades();

    for (uint8_t row = 0; row < ENEMY_ROWS; row++) {
        for (uint8_t col = 0; col < ENEMY_COLS; col++) {
            Entity* e = (Entity*)malloc(sizeof(Entity));
            if (e == NULL) return;

            e->type = ENTITY_ENEMY;
            e->sprite = &spr_enemy1;
            e->x = (int16_t)(START_X + ENEMY_SPACING * col);
            e->y = (int16_t)(START_Y + ENEMY_SPACING * row);
            e->dx = ENEMY_STEP_SIZE_X;
            e->dy = 0;

            enemies[row * ENEMY_COLS + col] = e;
        }
    }

    player.type = ENTITY_PLAYER;
    player.sprite = &spr_player;
    player.x = (int16_t)(END_X / 2 - scaled_sprite_width(&spr_player) / 2);
    player.y = 210;
    player.dx = 0;
    player.dy = 0;

    reset_timers();
}

static void reset_game() {
    state = GS_MENU;
    score = 0;

    reset_board();
}

static bool check_collision(Entity* a, Entity* b) {
    return a->x < b->x + entity_width(b) && a->x + entity_width(a) > b->x &&
           a->y < b->y + entity_height(b) && a->y + entity_height(a) > b->y;
}

static void destroy_barricade_block(uint8_t group, int8_t x, int8_t y) {
    if (barricades == NULL) return;
    if (group >= BARRICADE_GROUPS) return;
    if (x < 0 || x >= BARRICADE_COLS) return;
    if (y < 0 || y >= BARRICADE_ROWS) return;

    uint16_t index = barricade_index(group, (uint8_t)x, (uint8_t)y);
    Entity* e = barricades[index];
    if (e != NULL) {
        free(e);
        barricades[index] = NULL;
    }
}

static void explode_barricade(uint16_t hit_index) {
    uint8_t group = (uint8_t)(hit_index / BARRICADE_BLOCKS_PER_GROUP);
    uint8_t local_index = (uint8_t)(hit_index % BARRICADE_BLOCKS_PER_GROUP);
    uint8_t hit_x = local_index % BARRICADE_COLS;
    uint8_t hit_y = local_index / BARRICADE_COLS;

    for (uint8_t y = 0; y < EXPLOSION_ROWS; y++) {
        for (uint8_t x = 0; x < EXPLOSION_COLS; x++) {
            if (mask_explosion_shape[y][x] == 0) continue;

            destroy_barricade_block(group,
                                    (int8_t)(hit_x + x - EXPLOSION_COLS / 2),
                                    (int8_t)(hit_y + y - EXPLOSION_ROWS / 2));
        }
    }
}

static void create_bullet(uint16_t x, uint16_t y, int16_t dx, int16_t dy) {
    int8_t free_index = -1;
    for (uint8_t i = 0; i < MAX_BULLET_COUNT; i++) {
        if (bullets[i] == NULL) {
            free_index = i;
            break;
        }
    }

    if (free_index == -1) return;

    Entity* b = (Entity*)malloc(sizeof(Entity));
    if (b == NULL) return;

    b->type = ENTITY_BULLET;
    b->sprite = &spr_bullet;
    b->x = x;
    b->y = y;
    b->dx = dx;
    b->dy = dy;

    bullets[free_index] = b;
}

static void enemy_shoot() {
    bool found_front = false;
    Entity* front_enemies[ENEMY_COLS] = {NULL};

    for (int col = 0; col < ENEMY_COLS; col++) {
        front_enemies[col] = NULL;

        for (int row = 0; row < ENEMY_ROWS; row++) {
            Entity* e = enemies[row * ENEMY_COLS + col];

            if (e != NULL) {
                found_front = true;
                front_enemies[col] = e;
            }
        }
    }

    if (!found_front) return;

    Entity* shooter = NULL;
    do {
        shooter = front_enemies[random(0, ENEMY_COLS)];
    } while (shooter == NULL);

    create_bullet(shooter->x + entity_width(shooter) / 2 -
                      scaled_sprite_width(&spr_bullet) / 2,
                  shooter->y + entity_height(shooter) + 1, 0, BULLET_STEP_SIZE);
}

static void update_enemies() {
    static uint8_t current_sprite = 0;

    bool move_down = false;

    current_sprite ^= 1;

    // Move enemies - x
    for (uint16_t i = 0; i < ENEMY_COUNT; i++) {
        Entity* e = enemies[i];
        if (e == NULL) continue;

        if (current_sprite == 0) {
            e->sprite = &spr_enemy1;
        } else {
            e->sprite = &spr_enemy2;
        }

        e->x += e->dx;

        if ((e->dx > 0 && e->x + entity_width(e) >= END_X) ||
            (e->dx < 0 && e->x <= START_X)) {
            move_down = true;
        }
    }

    // Move enemies - y
    if (move_down) {
        for (uint16_t i = 0; i < ENEMY_COUNT; i++) {
            Entity* e = enemies[i];
            if (e == NULL) continue;

            e->dx = -e->dx;
            e->y += ENEMY_STEP_SIZE_Y;

            if (e->y > BARRICADES_Y - 12) {
                state = GS_GAME_OVER;
                return;
            }
        }
    }
}

static void update_bullets() {
    if (pending_enemy_removal_index != -1) {
        uint8_t enemy_index = (uint8_t)pending_enemy_removal_index;
        pending_enemy_removal_index = -1;

        free(enemies[enemy_index]);
        enemies[enemy_index] = NULL;

        if (living_enemies_count <= 0) {
            reset_board();
            return;
        }
    }

    for (uint8_t i = 0; i < MAX_BULLET_COUNT; i++) {
        Entity* b = bullets[i];

        if (b == NULL) continue;

        b->x += b->dx;
        b->y += b->dy;

        if (b->y + entity_height(b) < START_Y ||
            b->y > player.y + entity_height(&player)) {
            free(b);
            bullets[i] = NULL;
            continue;
        }

        if (check_collision(b, &player)) {
            state = GS_GAME_OVER;
            return;
        }

        for (uint8_t j = 0; j < ENEMY_COUNT; j++) {
            Entity* e = enemies[j];
            if (e == NULL) continue;

            if (check_collision(b, e)) {
                living_enemies_count--;
                score += ENEMY_VALUE;

                free(b);
                bullets[i] = NULL;

                pending_enemy_removal_index = j;
                state = GS_PAUSED;

                e->sprite = &spr_explosion;
                return;
            }
        }

        for (uint16_t j = 0; j < BARRICADES_COUNT; j++) {
            if (barricades == NULL) break;

            Entity* e = barricades[j];
            if (e == NULL) continue;

            if (check_collision(b, e)) {
                free(b);
                bullets[i] = NULL;

                explode_barricade(j);
                return;
            }
        }
    }
}

static void update_player() {
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
                    if (millis() - last_player_shot_tick > 200) {
                        last_player_shot_tick = millis();
                        create_bullet(
                            player.x + entity_width(&player) / 2 -
                                scaled_sprite_width(&spr_bullet) / 2,
                            player.y - scaled_sprite_height(&spr_bullet), 0,
                            -BULLET_STEP_SIZE);
                    }
                    break;
            }
        }
    }

    player.dx = 0;
    player.dy = 0;

    if (move_left && player.x > 0) {
        player.dx -= PLAYER_STEP_SIZE;
    }

    if (move_right && player.x + entity_width(&player) < END_X) {
        player.dx += PLAYER_STEP_SIZE;
    }

    player.x += player.dx;
    player.y += player.dy;

    if (player.x < 0) player.x = 0;

    int16_t max_player_x = END_X - entity_width(&player);
    if (player.x > max_player_x) player.x = max_player_x;
}

static void draw_score() {
    video_out.setCursor(START_X, 20);
    video_out.print("Score: ");
    video_out.print(score);
    video_out.drawFastHLine(0, 40, 250, 0xFF);
}

static void draw_entities(Entity** entities, uint32_t size) {
    if (entities == NULL) return;

    for (uint16_t i = 0; i < size; i++) {
        Entity* e = entities[i];
        if (e == NULL) continue;

        draw_sprite(e->sprite, e->x, e->y, ENTITY_SCALE);
    }
}

static void draw_player() {
    draw_sprite(player.sprite, player.x, player.y, ENTITY_SCALE);
}

static void draw_menu() {
    static uint32_t last_animation_tick = millis();
    static uint8_t spr = 1;

    if (millis() - last_animation_tick > 500) {
        last_animation_tick = millis();
        spr ^= 1;
    }

    if (spr == 1) {
        draw_sprite(&spr_enemy1, END_X / 2 - spr_enemy1.width * 6 / 2, 40, 6);
    } else {
        draw_sprite(&spr_enemy2, END_X / 2 - spr_enemy2.width * 6 / 2, 40, 6);
    }

    video_out.setCursor(45, 100);
    video_out.setTextSize(3);
    video_out.println("INVADERS!");
    video_out.setTextSize(2);
    video_out.setCursor(46, 140);
    video_out.println("Press any key");
}

void invaders_enter() { reset_game(); }

void invaders_exit() { free_resources(); }

void invaders_render() {
    video_out.waitForFrame();
    video_out.fillScreen(0);
    video_out.setTextColor(0xFF);
    video_out.setTextSize(2);

    switch (state) {
        case GS_GAME_OVER:
            draw_score();
            draw_player();
            draw_game_over();
            draw_entities(barricades, BARRICADES_COUNT);
            draw_entities(enemies, ENEMY_COUNT);
            draw_entities(bullets, MAX_BULLET_COUNT);
            break;

        case GS_PAUSED:
            if (paused_tick == -1) {
                paused_tick = millis();
            } else if (millis() - paused_tick > 200) {
                paused_tick = -1;
                state = GS_PLAYING;
            } else {
                // Prevent accumulating shots while paused
                last_player_shot_tick = millis();
            }

        case GS_PLAYING:
            if (state == GS_PLAYING) {
                update_player();
                update_bullets();
            }

            if (state == GS_PLAYING) {
                if (millis() - last_enemy_update_tick >
                    1000 * living_enemies_count / 60) {
                    last_enemy_update_tick = millis();
                    update_enemies();
                }

                if (millis() - last_enemy_shoot_tick > 2000) {
                    last_enemy_shoot_tick = millis();
                    enemy_shoot();
                }
            }

            draw_entities(barricades, BARRICADES_COUNT);
            draw_entities(enemies, ENEMY_COUNT);
            draw_entities(bullets, MAX_BULLET_COUNT);
            draw_score();
            draw_player();
            break;

        case GS_MENU:
            draw_menu();
            break;
    }
}

void invaders_handle_key(char read_key) {
    if (state == GS_MENU) {
        state = GS_PLAYING;

        return;
    }

    if (state == GS_GAME_OVER) {
        reset_game();
        return;
    }
}
