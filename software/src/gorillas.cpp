#include <stdlib.h>

#include <bitset>
#include <cmath>
#include <random>

#include "video_framebuffer.h"

#define GORILLA_BODY_WIDTH 19
#define GORILLA_BODY_HEIGHT 29
#define GORILLA_ARM_WIDTH 8
#define GORILLA_ARM_HEIGHT 11
#define BANANA_WIDTH 4
#define BANANA_HEIGHT 7
#define SUN_BODY_WIDTH 41
#define SUN_BODY_HEIGHT 30
#define SUN_MOUTH_REG_WIDTH 15
#define SUN_MOUTH_REG_HEIGHT 4
#define SUN_MOUTH_OPEN_WIDTH 7
#define SUN_MOUTH_OPEN_HEIGHT 5

#define RGB332(red, green, blue) \
    ((((red) & 0x07) << 5) | (((green) & 0x07) << 2) | ((blue) & 0x03))

#define GORILLA_COLOR 255
#define BANANA_COLOR 253
#define SUN_COLOR 180
#define HUD_COLOR 254
#define SKY_COLOR 0
#define EXPLOSION_COLOR RGB332(7, 1, 0)
#define UNLIT_WINDOW_COLOR RGB332(0, 0, 1)
#define LIT_WINDOW_COLOR RGB332(0, 1, 3)

#define SUN_X (screen_width / 2 - SUN_BODY_WIDTH / 2)
#define SUN_Y 30

#define TEXT_Y_START 25

#define SIMULATION_TICK_SPEED 20
#define SIMULATION_TIME_STEP 0.1f
#define SIMULATION_GRAVITY 9.8f
#define EXPLOSION_TIME_STEP 0.05f
#define EXPLOSION_DURATION 1.0f
#define BUILDING_EXPLOSION_RADIUS 5
#define GORILLA_EXPLOSION_RADIUS 30
#define PI_F 3.14159265358979323846f

#define GORILLA_CENTER_X(x) ((x) + GORILLA_BODY_WIDTH / 2)
#define GORILLA_CENTER_Y(y) ((y) - GORILLA_BODY_HEIGHT / 2)

#define GORILLA_ARM_OFFSET(up) ((up) ? -1 : 9)
#define GORILLA_LEFT_OVERHANG 4
#define GORILLA_RIGHT_OVERHANG (GORILLA_ARM_WIDTH - 3)

typedef enum {
    GS_READ_ANGLE,
    GS_READ_VELOCITY,
    GS_SIMULATE_BANANA,
    GS_EXPLODING,
    GS_GAMEOVER
} GameState;

typedef enum { PLAYER_ONE, PLAYER_TWO } Player;

typedef struct {
    uint16_t pos[2];
    bool arms[2];
    bool alive;
} PlayerState;

typedef struct {
    uint32_t last_tick;
    float time;
    int16_t previous_pos[2];
    bool has_previous_pos;
    uint8_t drawn_rotation;
} BananaState;

typedef struct {
    bool active;
    float time;
    int16_t pos[2];
    bool hit_gorilla;
    Player hit_player;
} ExplosionState;

typedef struct {
    uint32_t last_toss_tick;
    uint32_t last_win_tick;
    uint32_t last_sun_mouth_tick;
} AnimationState;

typedef struct {
    int16_t angle;
    int16_t velocity;
    Player current_player;
    int16_t wind;
    GameState state;
    uint16_t input;
} RoundState;

typedef struct {
    bool previous_copy_after_swap;
} ModeState;

const std::bitset<88> gorilla_arm_bits(
    "11111000011111000011111000111110000111110001111100011111001111100011111000"
    "11110000111000");
const std::bitset<551> gorilla_body_bits(
    "01111100000001111101111100000000011111111110000000001111111111000000000111"
    "11111110000000001111111111000000000111110111110000000111110011111100000111"
    "11100001111111111111000000111111111111000000011111111111100000001111111111"
    "11000000011111111111100000001111111111110000011000111110001110001111101110"
    "11111100011111101011111110001111111011111111000111111101111111100011111111"
    "11111111000111111111111111100000000111111000000000000111111110000000000011"
    "11111100000000001100100111000000000111111111100000000011000000110000000000"
    "111111110000000000011111111000000");

const std::bitset<28> banana_bits("1100111001110111011111101100");

const std::bitset<1230> sun_body_bits(
    "00000000000000000000100000000000000000000000000000000100000001000000010000"
    "00000000000000000000100000001000000100000000000000000000000000100000010000"
    "00100000000000000000011000000010000010000010000000010000000000001000000100"
    "11111110100000000100000000000000110000011111111111000000110000000000000000"
    "01011111111111111111001000000000000000000001111111111111111111100000000000"
    "01110000011111111111111111111100000001000000011111111111111111111111111100"
    "11110000000000001111111111111111111111111000000000000000111111111111111111"
    "11111110000000000000000111111111111111111111111100000000111111111111111111"
    "11111111111111111111111000000001111111110111110111111111000000000000000011"
    "11111100011100011111111000000000000000001111111101111101111111111000000000"
    "00011111111111111111111111111100111100000111000001111111111111111111110000"
    "00010000000000000111111111111111111110000000000000000000010111111111111111"
    "11001000000000000000001100000111111111110000001100000000000000100000010011"
    "11111010000000010000000000011000000010000010000010000000010000000000000000"
    "00100000010000001000000000000000000000000010000000100000010000000000000000"
    "00000000010000000100000001000000000000000000000000000000001000000000000000"
    "0000000000000000000000000100000000000000000000");
const std::bitset<60> sun_mouth_regular_bits(
    "000001111100000000110000011000011000000000110100000000000001");
const std::bitset<35> sun_mouth_open_bits(
    "00111001111111111111111111110011100");

static PlayerState player_one;
static PlayerState player_two;
static BananaState banana;
static ExplosionState explosion;
static AnimationState animations;
static RoundState game;
static ModeState mode;

static void reset_simulation() {
    banana.time = 0;
    banana.last_tick = 0;
    banana.has_previous_pos = false;
}

static void reset_explosion() {
    explosion.active = false;
    explosion.time = 0;
    explosion.pos[0] = 0;
    explosion.pos[1] = 0;
    explosion.hit_gorilla = false;
    explosion.hit_player = PLAYER_ONE;
}

static void reset_game() {
    game.current_player = PLAYER_ONE;
    game.state = GS_READ_ANGLE;
    game.input = 0;
    player_one.arms[0] = false;
    player_one.arms[1] = false;
    player_two.arms[0] = false;
    player_two.arms[1] = false;
    player_one.alive = true;
    player_two.alive = true;
    game.wind = 0;
    animations.last_toss_tick = 0;
    animations.last_win_tick = 0;
    animations.last_sun_mouth_tick = 0;
    reset_explosion();
    reset_simulation();
}

static void switch_player() {
    if (game.current_player == PLAYER_ONE) {
        game.current_player = PLAYER_TWO;
    } else {
        game.current_player = PLAYER_ONE;
    }
}

static PlayerState* get_player_state(Player player) {
    return player == PLAYER_ONE ? &player_one : &player_two;
}

static Player get_opponent(Player player) {
    return player == PLAYER_ONE ? PLAYER_TWO : PLAYER_ONE;
}

static Player player_from_hit_x(int16_t x) {
    return x > screen_width / 2 ? PLAYER_TWO : PLAYER_ONE;
}

static int16_t calculate_banana_y_position(uint16_t start_y) {
    float angle_radians = (float)game.angle / 180.0f * PI_F;
    float initial_y_velocity = std::sin(angle_radians) * (float)game.velocity;

    return (int16_t)(start_y + (-1.0f * initial_y_velocity * banana.time) +
                     (0.5f * SIMULATION_GRAVITY * banana.time * banana.time));
}

static int16_t calculate_banana_x_position(uint16_t start_x) {
    float angle_radians = game.angle / 180.0f * PI_F;
    float initial_x_velocity = std::cos(angle_radians) * game.velocity;
    if (game.current_player == PLAYER_TWO) initial_x_velocity *= -1.0f;

    return (int16_t)(start_x + initial_x_velocity * banana.time +
                     0.5f * ((float)game.wind / 5.0f) * banana.time *
                         banana.time);
}

template <std::size_t N>
static void draw_bitset(const std::bitset<N>* data, int16_t x, int16_t y,
                        uint8_t width, uint8_t height, bool flip_x, bool flip_y,
                        uint8_t color) {
    for (uint16_t i = 0; i < data->size(); ++i) {
        uint8_t xx = i % width;
        uint8_t yy = i / width;

        if (flip_x) xx = width - 1 - xx;
        if (flip_y) yy = height - 1 - yy;

        if (data->test(i)) {
            video_out.drawPixel(x + xx, y + yy, color);
        }
    }
}

static void draw_gorilla(int16_t x, int16_t y, bool left_arm_up,
                         bool right_arm_up) {
    int8_t previous_left_arm_offset = GORILLA_ARM_OFFSET(!left_arm_up);
    int8_t previous_right_arm_offset = GORILLA_ARM_OFFSET(!right_arm_up);
    int8_t left_arm_offset = GORILLA_ARM_OFFSET(left_arm_up);
    int8_t right_arm_offset = GORILLA_ARM_OFFSET(right_arm_up);

    draw_bitset(&gorilla_arm_bits, x - 4, y + previous_left_arm_offset,
                GORILLA_ARM_WIDTH, GORILLA_ARM_HEIGHT, false, !left_arm_up, 0);
    draw_bitset(&gorilla_arm_bits, x + GORILLA_BODY_WIDTH - 3,
                y + previous_right_arm_offset, GORILLA_ARM_WIDTH,
                GORILLA_ARM_HEIGHT, true, !right_arm_up, 0);

    draw_bitset(&gorilla_body_bits, x, y, GORILLA_BODY_WIDTH,
                GORILLA_BODY_HEIGHT, false, false, GORILLA_COLOR);

    draw_bitset(&gorilla_arm_bits, x - 4, y + left_arm_offset,
                GORILLA_ARM_WIDTH, GORILLA_ARM_HEIGHT, false, left_arm_up,
                GORILLA_COLOR);

    draw_bitset(&gorilla_arm_bits, x + GORILLA_BODY_WIDTH - 3,
                y + right_arm_offset, GORILLA_ARM_WIDTH, GORILLA_ARM_HEIGHT,
                true, right_arm_up, GORILLA_COLOR);
}

static void draw_house(int16_t x, int16_t y, uint8_t width, uint8_t height,
                       uint8_t color) {
    video_out.fillRect(x, y, width, height, color);

    // Draw windows
    for (uint8_t xx = 3; xx <= width - 5; xx += 8) {
        for (uint8_t yy = 3; yy <= height - 11; yy += 15) {
            uint8_t c = random(0, 3) >= 1 ? LIT_WINDOW_COLOR : UNLIT_WINDOW_COLOR;

            video_out.fillRect(x + xx, y + yy, 4, 10, c);
        }
    }
}

static uint8_t generate_house_color() {
    static const uint8_t house_colors[] = {
        RGB332(2, 3, 1), RGB332(3, 3, 2), RGB332(3, 4, 1), RGB332(4, 3, 2),
        RGB332(4, 5, 1), RGB332(5, 4, 2), RGB332(5, 6, 1), RGB332(6, 5, 2),
        RGB332(6, 7, 1), RGB332(7, 6, 0), RGB332(7, 7, 0),
    };

    return house_colors[random(0,
                               sizeof(house_colors) / sizeof(house_colors[0]))];
}

static uint16_t calculate_gorilla_y_position(uint16_t x) {
    uint8_t** framebuffer = video_framebuffer_lines();

    if (framebuffer == NULL) return screen_height;
    if (screen_width <= GORILLA_BODY_WIDTH) return screen_height;

    uint16_t max_x = screen_width - GORILLA_BODY_WIDTH;
    if (x > max_x) return screen_height;

    int16_t center_x = x + GORILLA_BODY_WIDTH / 2;
    int16_t sample_offsets[] = {0, 2, -2, 4, -4, 6, -6};

    for (uint8_t i = 0; i < 7; i++) {
        int16_t sample_x = center_x + sample_offsets[i];
        if (sample_x < 0 || sample_x >= screen_width) continue;

        uint16_t y = 0;
        while (y < screen_height && framebuffer[y][sample_x] == 0) {
            y++;
        }

        if (y < screen_height) return y;
    }

    return screen_height;
}

static bool find_banana_collision(int16_t x0, int16_t y0, int16_t x1,
                                  int16_t y1, int16_t* hit_x, int16_t* hit_y,
                                  bool* hit_sun) {
    uint8_t** framebuffer = video_framebuffer_lines();
    if (framebuffer == NULL) return false;

    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t steps = dx > dy ? dx : dy;
    if (steps == 0) steps = 1;

    for (int16_t i = 0; i <= steps; i++) {
        int16_t x = x0 + (int32_t)(x1 - x0) * i / steps;
        int16_t y = y0 + (int32_t)(y1 - y0) * i / steps;

        if (x < 0 || x >= screen_width) continue;
        if (y < 0 || y >= screen_height) continue;

        uint8_t color = framebuffer[y][x];
        if (color == 0) continue;
        if (color == HUD_COLOR) continue;

        if (color == SUN_COLOR) {
            *hit_sun = true;
            continue;
        }

        *hit_x = x;
        *hit_y = y;
        return true;
    }

    return false;
}

static uint16_t adjust_gorilla_x_position(uint16_t x) {
    uint8_t** framebuffer = video_framebuffer_lines();

    uint16_t min_x = GORILLA_LEFT_OVERHANG;
    uint16_t max_x = screen_width - GORILLA_BODY_WIDTH - GORILLA_RIGHT_OVERHANG;

    if (x < min_x) x = min_x;
    if (x > max_x) x = max_x;

    for (uint16_t test_x = x; test_x <= max_x; test_x++) {
        uint16_t y = calculate_gorilla_y_position(test_x);
        if (y >= screen_height) continue;

        bool supported = true;
        for (int16_t xx = -GORILLA_LEFT_OVERHANG;
             xx < GORILLA_BODY_WIDTH + GORILLA_RIGHT_OVERHANG; xx++) {
            if (framebuffer[y][test_x + xx] == SKY_COLOR) {
                supported = false;
                break;
            }
        }

        if (supported) return test_x;
    }

    return x;
}

static void draw_wind() {
    if (game.wind == 0) return;

    int16_t x = 180;
    int16_t y = TEXT_Y_START;
    int16_t wind_line = (int32_t)game.wind * 3 * screen_width / 320;

    if (wind_line == 0) wind_line = game.wind > 0 ? 1 : -1;

    int16_t end_x = x + wind_line;
    int16_t arrow_dir = game.wind > 0 ? -2 : 2;

    video_out.drawLine(x, y, end_x, y, HUD_COLOR);
    video_out.drawLine(end_x, y, end_x + arrow_dir, y - 2, HUD_COLOR);
    video_out.drawLine(end_x, y, end_x + arrow_dir, y + 2, HUD_COLOR);
}

static void clear_input_area() {
    video_out.fillRect(0, TEXT_Y_START, SUN_X, 10, SKY_COLOR);
}

static bool sun_mouth_is_open() {
    return animations.last_sun_mouth_tick != 0 &&
           millis() - animations.last_sun_mouth_tick < 1000;
}

static void draw_sun() {
    draw_bitset(&sun_body_bits, SUN_X, SUN_Y, SUN_BODY_WIDTH, SUN_BODY_HEIGHT,
                false, false, SUN_COLOR);

    uint8_t mouth_y = SUN_Y + 18;

    if (sun_mouth_is_open()) {
        uint16_t mouth_x = screen_width / 2 - SUN_MOUTH_OPEN_WIDTH / 2;

        draw_bitset(&sun_mouth_open_bits, mouth_x, mouth_y,
                    SUN_MOUTH_OPEN_WIDTH, SUN_MOUTH_OPEN_HEIGHT, false, false,
                    SKY_COLOR);
    } else {
        uint16_t mouth_x = screen_width / 2 - SUN_MOUTH_REG_WIDTH / 2;

        draw_bitset(&sun_mouth_regular_bits, mouth_x, mouth_y,
                    SUN_MOUTH_REG_WIDTH, SUN_MOUTH_REG_HEIGHT, false, false,
                    SKY_COLOR);
    }
}

static void draw_read_angle() {
    if (game.state != GS_READ_ANGLE) return;

    video_out.print("Angle: ");
    if (game.input != 0) {
        video_out.println(game.input);
    }
}

static void draw_read_velocity() {
    if (game.state != GS_READ_VELOCITY) return;

    video_out.print("Velocity: ");
    if (game.input != 0) {
        video_out.println(game.input);
    }
}

static void draw_banana(int16_t x, int16_t y, uint8_t rotation, uint8_t color) {
    int16_t rotated_width = rotation % 2 == 0 ? BANANA_WIDTH : BANANA_HEIGHT;
    int16_t rotated_height = rotation % 2 == 0 ? BANANA_HEIGHT : BANANA_WIDTH;

    int16_t target_x = x - rotated_width / 2;
    int16_t target_y = y - rotated_height / 2;

    int16_t render_x;
    int16_t render_y;

    switch (rotation) {
        case 1:
            render_x = target_y;
            render_y = screen_width - target_x - BANANA_HEIGHT;
            break;
        case 2:
            render_x = screen_width - target_x - BANANA_WIDTH;
            render_y = screen_height - target_y - BANANA_HEIGHT;
            break;
        case 3:
            render_x = screen_height - target_y - BANANA_WIDTH;
            render_y = target_x;
            break;
        default:
            render_x = target_x;
            render_y = target_y;
            break;
    }

    video_out.setRotation(rotation);
    draw_bitset(&banana_bits, render_x, render_y, BANANA_WIDTH, BANANA_HEIGHT,
                false, false, color);
    video_out.setRotation(0);
}

static void clear_drawn_banana() {
    if (!banana.has_previous_pos) return;

    draw_banana(banana.previous_pos[0], banana.previous_pos[1],
                banana.drawn_rotation, SKY_COLOR);
}

static void draw_banana(int16_t x, int16_t y) {
    uint8_t rotation = ((uint32_t)(banana.time * 5)) % 4;

    draw_banana(x, y, rotation, BANANA_COLOR);
    banana.drawn_rotation = rotation;
}

static bool is_gorilla_hit(int16_t x, int16_t y) {
    uint8_t** framebuffer = video_framebuffer_lines();
    if (framebuffer == NULL) return false;

    return x >= 0 && x < screen_width && y >= 0 && y < screen_height &&
           framebuffer[y][x] == GORILLA_COLOR;
}

static void set_winner_pose(Player winner) {
    // Reset toss tick to prevent arm resetting overwriting pose
    animations.last_toss_tick = 0;
    PlayerState* winner_state = get_player_state(winner);
    winner_state->arms[0] = true;
    winner_state->arms[1] = false;
}

static void start_explosion(int16_t x, int16_t y) {
    explosion.active = true;
    explosion.time = 0;
    explosion.hit_gorilla = is_gorilla_hit(x, y);
    explosion.hit_player = player_from_hit_x(x);
    game.state = GS_EXPLODING;

    if (explosion.hit_gorilla) {
        PlayerState* hit_player_state = get_player_state(explosion.hit_player);
        hit_player_state->alive = false;
        set_winner_pose(get_opponent(explosion.hit_player));

        explosion.pos[0] = GORILLA_CENTER_X(hit_player_state->pos[0]);
        explosion.pos[1] = GORILLA_CENTER_Y(hit_player_state->pos[1]);
    } else {
        explosion.pos[0] = x;
        explosion.pos[1] = y;
    }
}

static void draw_explosion_frame(uint8_t final_radius) {
    video_out.fillCircle(explosion.pos[0], explosion.pos[1], final_radius,
                         SKY_COLOR);

    int16_t radius = (int16_t)(final_radius * (1.0 - explosion.time));
    uint16_t color = explosion.time < 0.1 ? SKY_COLOR : EXPLOSION_COLOR;

    video_out.fillCircle(explosion.pos[0], explosion.pos[1], radius, color);
}

static void finish_explosion() {
    // Overtwrite animation residual artifact
    video_out.fillCircle(explosion.pos[0], explosion.pos[1],
                         BUILDING_EXPLOSION_RADIUS, SKY_COLOR);

    bool hit_gorilla = explosion.hit_gorilla;

    reset_explosion();
    reset_simulation();

    if (hit_gorilla) {
        game.state = GS_GAMEOVER;
    } else {
        game.state = GS_READ_ANGLE;
        switch_player();
    }
}

static void update_explosion() {
    if (!explosion.active) return;

    if (explosion.hit_gorilla) {
        draw_explosion_frame(GORILLA_EXPLOSION_RADIUS);
    } else {
        draw_explosion_frame(BUILDING_EXPLOSION_RADIUS);
    }

    explosion.time += EXPLOSION_TIME_STEP;
    if (explosion.time >= EXPLOSION_DURATION) {
        finish_explosion();
    }
}

static void simulate_banana() {
    if (game.state != GS_SIMULATE_BANANA) return;

    uint16_t start_x;
    uint16_t start_y;
    if (game.current_player == PLAYER_ONE) {
        start_x = player_one.pos[0];
        start_y = player_one.pos[1] - GORILLA_BODY_HEIGHT - 5;
    } else {
        start_x = player_two.pos[0] + GORILLA_BODY_WIDTH;
        start_y = player_two.pos[1] - GORILLA_BODY_HEIGHT - 5;
    }

    if (millis() - banana.last_tick > SIMULATION_TICK_SPEED) {
        banana.last_tick = millis();
        banana.time += SIMULATION_TIME_STEP;
    }

    int16_t x = calculate_banana_x_position(start_x);
    int16_t y = calculate_banana_y_position(start_y);

    if (x < 0 || x >= screen_width || y >= screen_height || y < -500) {
        game.state = GS_READ_ANGLE;
        reset_simulation();
        switch_player();
        return;
    }

    int16_t collision_start_x =
        banana.has_previous_pos ? banana.previous_pos[0] : x;
    int16_t collision_start_y =
        banana.has_previous_pos ? banana.previous_pos[1] : y;
    bool hit_sun = false;

    int16_t impact_x;
    int16_t impact_y;

    if (find_banana_collision(collision_start_x, collision_start_y, x, y,
                              &impact_x, &impact_y, &hit_sun)) {
        start_explosion(impact_x, impact_y);
        return;
    }

    if (hit_sun) {
        animations.last_sun_mouth_tick = millis();
    }

    banana.has_previous_pos = true;
    banana.previous_pos[0] = x;
    banana.previous_pos[1] = y;

    if (y < 0) return;

    draw_banana(x, y);
}

static void generate_houses() {
    uint16_t x = 0;

    while (x < screen_width) {
        uint8_t width = random(30, 61);
        uint8_t height = random(30, 150);
        uint8_t color = generate_house_color();

        draw_house(x, screen_height - height, width, height, color);

        x += width + 2;
    }
}

static void generate_wind() {
    game.wind = random(1, 11) - 5;

    if (random(1, 4) == 1) {
        if (game.wind > 0) {
            game.wind += random(1, 11);
        } else {
            game.wind -= random(1, 11);
        }
    }
}

static void place_gorillas() {
    player_one.pos[0] = random(10, 50);
    player_one.pos[0] = adjust_gorilla_x_position(player_one.pos[0]);
    player_one.pos[1] = calculate_gorilla_y_position(player_one.pos[0]);

    player_two.pos[0] = random(screen_width - 80, screen_width - 40);
    player_two.pos[0] = adjust_gorilla_x_position(player_two.pos[0]);
    player_two.pos[1] = calculate_gorilla_y_position(player_two.pos[0]);
}

static void generate_level() {
    video_out.fillScreen(0);
    generate_houses();
    place_gorillas();
    generate_wind();
}

void gorillas_enter() {
    mode.previous_copy_after_swap = video_out.copyAfterSwap;
    video_out.copyAfterSwap = true;
    reset_game();
    generate_level();
}

void gorillas_exit() {
    video_out.copyAfterSwap = mode.previous_copy_after_swap;
}

void gorillas_render() {
    video_out.waitForFrame();
    video_out.setTextSize(1);
    video_out.setTextColor(HUD_COLOR);

    clear_input_area();
    clear_drawn_banana();
    draw_sun();

    if (game.state != GS_GAMEOVER && animations.last_toss_tick != 0 &&
        millis() - animations.last_toss_tick > 500) {
        player_one.arms[0] = false;
        player_two.arms[1] = false;
    }

    if (game.state == GS_GAMEOVER &&
        millis() - animations.last_win_tick > 500) {
        animations.last_win_tick = millis();

        player_two.arms[0] = !player_two.arms[0];
        player_two.arms[1] = !player_two.arms[1];

        player_one.arms[0] = !player_one.arms[0];
        player_one.arms[1] = !player_one.arms[1];
    }

    if (player_one.alive) {
        draw_gorilla(player_one.pos[0], player_one.pos[1] - GORILLA_BODY_HEIGHT,
                     player_one.arms[0], player_one.arms[1]);
    }

    if (player_two.alive) {
        draw_gorilla(player_two.pos[0], player_two.pos[1] - GORILLA_BODY_HEIGHT,
                     player_two.arms[0], player_two.arms[1]);
    }

    if (explosion.active) update_explosion();

    video_out.setCursor(0, TEXT_Y_START);
    if (game.state != GS_EXPLODING) {
        draw_read_angle();
        draw_read_velocity();
        simulate_banana();
    }
    draw_wind();
    draw_sun();
}

void gorillas_handle_key(char read_key) {
    if (game.state == GS_GAMEOVER) {
        reset_game();
        generate_level();
        return;
    }

    if (game.state != GS_READ_ANGLE && game.state != GS_READ_VELOCITY) return;

    if (read_key >= '0' && read_key <= '9') {
        game.input *= 10;
        game.input += read_key - '0';
    } else if (read_key == '<') {
        game.input /= 10;
    } else if (read_key == '=') {
        if (game.state == GS_READ_ANGLE) {
            game.angle = game.input;
            game.state = GS_READ_VELOCITY;
        } else if (game.state == GS_READ_VELOCITY) {
            game.velocity = game.input;
            game.state = GS_SIMULATE_BANANA;

            animations.last_toss_tick = millis();
            if (game.current_player == PLAYER_ONE) {
                player_one.arms[0] = true;
            } else {
                player_two.arms[1] = true;
            }
        }

        game.input = 0;
    } else if (read_key == '(') {
        reset_game();
        generate_level();
        return;
    }
}
