#include <stdlib.h>

#include <bitset>
#include <cmath>
#include <cstring>
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

#define SUN_X (screen_width / 2 - SUN_BODY_WIDTH / 2)
#define SUN_Y 30

#define TEXT_Y_START 25

#define SIMULATION_TICK_SPEED 20
#define SIMULATION_GRAVITY 9.8f
#define PI_F 3.14159265358979323846f

#define GORILLA_CENTER_X(x) ((x) + GORILLA_BODY_WIDTH / 2)
#define GORILLA_CENTER_Y(y) ((y) - GORILLA_BODY_HEIGHT / 2)

#define GORILLA_ARM_OFFSET(up) ((up) ? -1 : 9)
#define GORILLA_LEFT_OVERHANG 4
#define GORILLA_RIGHT_OVERHANG (GORILLA_ARM_WIDTH - 3)

typedef enum {
    GS_SIMULATE_BANANA,
    GS_READ_ANGLE,
    GS_READ_VELOCITY,
    GS_GAMEOVER
} GameState;

typedef enum { PLAYER_ONE, PLAYER_TWO } Player;

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

static uint8_t* level_data = NULL;
static int16_t angle;
static int16_t velocity;
static Player current_player;
static GameState state;
static uint16_t player_one_pos[2];
static uint16_t player_two_pos[2];
static bool player_one_arms[2];
static bool player_two_arms[2];
static uint32_t last_toss_animation_tick;
static uint32_t last_win_animation_tick;
static uint32_t last_sun_mouth_tick;
static bool player_one_alive;
static bool player_two_alive;
static int16_t wind;

static uint16_t input;
static float simulation_time;
static uint32_t last_simulation_tick;
static int16_t impact_pos[2];
static int16_t previous_banana_pos[2];
static bool has_previous_banana_pos;

static bool allocate_level_data() {
    level_data = (uint8_t*)malloc((uint32_t)screen_width * screen_height);

    return level_data != NULL;
}

static void copy_framebuffer_into_level() {
    if (level_data == NULL) return;

    uint8_t** framebuffer = video_framebuffer_lines();
    if (framebuffer == NULL) return;

    for (uint16_t y = 0; y < screen_height; y++) {
        std::memcpy(level_data + (uint32_t)y * screen_width, framebuffer[y],
                    screen_width);
    }
}

static void copy_level_into_framebuffer() {
    if (level_data == NULL) return;

    uint8_t** framebuffer = video_framebuffer_lines();
    if (framebuffer == NULL) return;

    for (uint16_t y = 0; y < screen_height; y++) {
        std::memcpy(framebuffer[y], level_data + (uint32_t)y * screen_width,
                    screen_width);
    }
}

static void free_level() {
    if (level_data == NULL) return;
    free(level_data);
    level_data = NULL;
}

static void reset_simulation() {
    simulation_time = 0;
    last_simulation_tick = 0;
    has_previous_banana_pos = false;
}

static void reset_impact() {
    impact_pos[0] = -1;
    impact_pos[1] = -1;
}

static void reset_game() {
    free_level();

    current_player = PLAYER_ONE;
    state = GS_READ_ANGLE;
    input = 0;
    player_one_arms[0] = false;
    player_one_arms[1] = false;
    player_two_arms[0] = false;
    player_two_arms[1] = false;
    player_one_alive = true;
    player_two_alive = true;
    wind = 0;
    last_toss_animation_tick = 0;
    last_win_animation_tick = 0;
    last_sun_mouth_tick = 0;
    reset_simulation();
    reset_impact();

    allocate_level_data();
}

static void switch_player() {
    if (current_player == PLAYER_ONE) {
        current_player = PLAYER_TWO;
    } else {
        current_player = PLAYER_ONE;
    }
}

static bool banana_in_sun(int16_t x, int16_t y) {
    return x >= SUN_X && x < SUN_X + SUN_BODY_WIDTH && y >= SUN_Y &&
           y < SUN_Y + SUN_BODY_HEIGHT;
}

static int16_t calculate_banana_y_position(uint16_t start_y) {
    float angle_radians = (float)angle / 180.0f * PI_F;
    float initial_y_velocity = std::sin(angle_radians) * (float)velocity;

    return (int16_t)(start_y + (-1.0f * initial_y_velocity * simulation_time) +
                     (0.5f * SIMULATION_GRAVITY * simulation_time *
                      simulation_time));
}

static int16_t calculate_banana_x_position(uint16_t start_x) {
    float angle_radians = angle / 180.0f * PI_F;
    float initial_x_velocity = std::cos(angle_radians) * velocity;
    if (current_player == PLAYER_TWO) initial_x_velocity *= -1.0f;

    return (int16_t)(start_x + initial_x_velocity * simulation_time +
                     0.5f * ((float)wind / 5.0f) * simulation_time *
                         simulation_time);
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
                GORILLA_BODY_HEIGHT, false, false, 0xFF);

    draw_bitset(&gorilla_arm_bits, x - 4, y + left_arm_offset,
                GORILLA_ARM_WIDTH, GORILLA_ARM_HEIGHT, false, left_arm_up,
                0xFF);

    draw_bitset(&gorilla_arm_bits, x + GORILLA_BODY_WIDTH - 3,
                y + right_arm_offset, GORILLA_ARM_WIDTH, GORILLA_ARM_HEIGHT,
                true, right_arm_up, 0xFF);
}

static void draw_house(int16_t x, int16_t y, uint8_t width, uint8_t height,
                       uint8_t color) {
    video_out.fillRect(x, y, width, height, color);

    // Draw windows
    for (uint8_t xx = 3; xx <= width - 5; xx += 8) {
        for (uint8_t yy = 3; yy <= height - 11; yy += 15) {
            uint8_t c = random(0, 3) >= 1 ? 10 : 0;

            video_out.fillRect(x + xx, y + yy, 4, 10, c);
        }
    }
}

static void generate_houses() {
    uint16_t x = 0;

    while (x < screen_width) {
        uint8_t width = random(30, 61);
        uint8_t height = random(30, 150);
        // Upper bound is 255 exclusive. 255 is reserved for gorillas
        uint8_t color = random(210, 255);

        draw_house(x, screen_height - height, width, height, color);

        x += width + 2;
    }
}

static void generate_wind() {
    wind = random(1, 11) - 5;

    if (random(1, 4) == 1) {
        if (wind > 0) {
            wind += random(1, 11);
        } else {
            wind -= random(1, 11);
        }
    }
}

static void generate_level() {
    video_out.fillScreen(0);
    generate_houses();
    place_gorillas();
    generate_wind();
    copy_framebuffer_into_level();
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
                                  int16_t y1, int16_t* hit_x, int16_t* hit_y) {
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

        if (framebuffer[y][x] != 0) {
            *hit_x = x;
            *hit_y = y;
            return true;
        }
    }

    return false;
}

static void simulate_banana() {
    if (state != GS_SIMULATE_BANANA) return;

    uint16_t start_x;
    uint16_t start_y;
    if (current_player == PLAYER_ONE) {
        start_x = player_one_pos[0];
        start_y = player_one_pos[1] - GORILLA_BODY_HEIGHT - 5;
    } else {
        start_x = player_two_pos[0] + GORILLA_BODY_WIDTH;
        start_y = player_two_pos[1] - GORILLA_BODY_HEIGHT - 5;
    }

    if (millis() - last_simulation_tick > SIMULATION_TICK_SPEED) {
        last_simulation_tick = millis();
        simulation_time += 0.1f;
    }

    int16_t x = calculate_banana_x_position(start_x);
    int16_t y = calculate_banana_y_position(start_y);

    if (x < 0 || x >= screen_width || y >= screen_height || y < -500) {
        state = GS_READ_ANGLE;
        reset_simulation();
        switch_player();
        return;
    }

    if (banana_in_sun(x, y)) {
        last_sun_mouth_tick = millis();
    }

    int16_t collision_start_x =
        has_previous_banana_pos ? previous_banana_pos[0] : x;
    int16_t collision_start_y =
        has_previous_banana_pos ? previous_banana_pos[1] : y;

    if (find_banana_collision(collision_start_x, collision_start_y, x, y,
                              &impact_pos[0], &impact_pos[1])) {
        state = GS_READ_ANGLE;
        reset_simulation();
        switch_player();
        return;
    }

    has_previous_banana_pos = true;
    previous_banana_pos[0] = x;
    previous_banana_pos[1] = y;

    if (y < 0) return;

    draw_banana(x, y);
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
            if (framebuffer[y][test_x + xx] == 0) {
                supported = false;
                break;
            }
        }

        if (supported) return test_x;
    }

    return x;
}

static void place_gorillas() {
    player_one_pos[0] = random(10, 50);
    player_one_pos[0] = adjust_gorilla_x_position(player_one_pos[0]);
    player_one_pos[1] = calculate_gorilla_y_position(player_one_pos[0]);

    player_two_pos[0] = random(screen_width - 80, screen_width - 40);
    player_two_pos[0] = adjust_gorilla_x_position(player_two_pos[0]);
    player_two_pos[1] = calculate_gorilla_y_position(player_two_pos[0]);
}

static void draw_wind() {
    if (wind == 0) return;

    int16_t x = 180;
    int16_t y = TEXT_Y_START;
    int16_t wind_line = (int32_t)wind * 3 * screen_width / 320;

    if (wind_line == 0) wind_line = wind > 0 ? 1 : -1;

    int16_t end_x = x + wind_line;
    int16_t arrow_dir = wind > 0 ? -2 : 2;

    video_out.drawLine(x, y, end_x, y, 0xFF);
    video_out.drawLine(end_x, y, end_x + arrow_dir, y - 2, 0xFF);
    video_out.drawLine(end_x, y, end_x + arrow_dir, y + 2, 0xFF);
}

static void draw_sun() {
    if (state == GS_GAMEOVER) return;

    draw_bitset(&sun_body_bits, SUN_X, SUN_Y, SUN_BODY_WIDTH, SUN_BODY_HEIGHT,
                false, false, 180);

    uint8_t mouth_y = SUN_Y + 18;

    if (millis() - last_sun_mouth_tick < 1000) {
        uint16_t mouth_x = screen_width / 2 - SUN_MOUTH_OPEN_WIDTH / 2;

        draw_bitset(&sun_mouth_open_bits, mouth_x, mouth_y,
                    SUN_MOUTH_OPEN_WIDTH, SUN_MOUTH_OPEN_HEIGHT, false, false,
                    0);
    } else {
        uint16_t mouth_x = screen_width / 2 - SUN_MOUTH_REG_WIDTH / 2;

        draw_bitset(&sun_mouth_regular_bits, mouth_x, mouth_y,
                    SUN_MOUTH_REG_WIDTH, SUN_MOUTH_REG_HEIGHT, false, false, 0);
    }
}

static void draw_read_angle() {
    if (state != GS_READ_ANGLE) return;

    video_out.print("Angle: ");
    if (input != 0) {
        video_out.println(input);
    }
}

static void draw_read_velocity() {
    if (state != GS_READ_VELOCITY) return;

    video_out.print("Velocity: ");
    if (input != 0) {
        video_out.println(input);
    }
}

static void draw_banana(int16_t x, int16_t y) {
    uint8_t rotation = ((uint32_t)(simulation_time * 5)) % 4;

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
                false, false, 0xFF);
    video_out.setRotation(0);
}

static void draw_gameover() {
    if (state != GS_GAMEOVER) return;

    video_out.setCursor(60, 40);
    video_out.setTextSize(2);
    video_out.println("GAME OVER!");
    video_out.setTextSize(1);
}

void gorillas_enter() {
    reset_game();
    generate_level();
}

void gorillas_exit() { free_level(); }

void gorillas_render() {
    video_out.waitForFrame();
    video_out.fillScreen(0);
    video_out.setTextSize(1);
    video_out.setTextColor(0xFF);

    copy_level_into_framebuffer();

    if (state != GS_GAMEOVER && last_toss_animation_tick != 0 &&
        millis() - last_toss_animation_tick > 500) {
        player_one_arms[0] = false;
        player_two_arms[1] = false;
    }

    if (state == GS_GAMEOVER && millis() - last_win_animation_tick > 500) {
        last_win_animation_tick = millis();

        player_two_arms[0] = !player_two_arms[0];
        player_two_arms[1] = !player_two_arms[1];

        player_one_arms[0] = !player_one_arms[0];
        player_one_arms[1] = !player_one_arms[1];
    }

    if (player_one_alive) {
        draw_gorilla(player_one_pos[0], player_one_pos[1] - GORILLA_BODY_HEIGHT,
                     player_one_arms[0], player_one_arms[1]);
    }

    if (player_two_alive) {
        draw_gorilla(player_two_pos[0], player_two_pos[1] - GORILLA_BODY_HEIGHT,
                     player_two_arms[0], player_two_arms[1]);
    }

    // Perform explosion
    if (impact_pos[0] != -1) {
        uint8_t** framebuffer = video_framebuffer_lines();
        int16_t x = impact_pos[0];
        int16_t y = impact_pos[1];

        // If gorilla hit
        if (framebuffer[y][x] == 0xFF) {
            // Decide whether right or left gorilla (player 1 or player 2)
            if (x > screen_width / 2) {
                video_out.fillCircle(GORILLA_CENTER_X(player_two_pos[0]),
                                     GORILLA_CENTER_Y(player_two_pos[1]), 25,
                                     0);
                player_two_alive = false;
                player_one_arms[0] = true;
                player_one_arms[1] = false;
            } else {
                video_out.fillCircle(GORILLA_CENTER_X(player_one_pos[0]),
                                     GORILLA_CENTER_Y(player_one_pos[1]), 25,
                                     0);
                player_one_alive = false;
                player_two_arms[0] = true;
                player_two_arms[1] = false;
            }

            state = GS_GAMEOVER;
        } else {
            video_out.fillCircle(x, y, 5, 0);
        }

        impact_pos[0] = -1;
        impact_pos[1] = -1;
    }

    copy_framebuffer_into_level();

    video_out.setCursor(0, TEXT_Y_START);
    draw_read_angle();
    draw_read_velocity();
    simulate_banana();
    draw_gameover();
    draw_wind();
    draw_sun();
}

void gorillas_handle_key(char read_key) {
    if (state == GS_GAMEOVER) {
        reset_game();
        generate_level();
        return;
    }

    if (state != GS_READ_ANGLE && state != GS_READ_VELOCITY) return;

    if (read_key >= '0' && read_key <= '9') {
        input *= 10;
        input += read_key - '0';
    } else if (read_key == '<') {
        input /= 10;
    } else if (read_key == '=') {
        if (state == GS_READ_ANGLE) {
            angle = input;
            state = GS_READ_VELOCITY;
        } else if (state == GS_READ_VELOCITY) {
            velocity = input;
            state = GS_SIMULATE_BANANA;

            last_toss_animation_tick = millis();
            if (current_player == PLAYER_ONE) {
                player_one_arms[0] = true;
            } else {
                player_two_arms[1] = true;
            }
        }

        input = 0;
    }
}
