#include <stdlib.h>

#include <bitset>
#include <cstring>
#include <random>

#include "video_framebuffer.h"

#define GORILLA_BODY_WIDTH 19
#define GORILLA_BODY_HEIGHT 29
#define GORILLA_ARM_WIDTH 8
#define GORILLA_ARM_HEIGHT 11

#define MAX_INPUT_SIZE 5
#define TEXT_Y_START 15

typedef enum { GS_SIMULATE_BANANA, GS_READ_ANGLE, GS_READ_VELOCITY } GameState;

typedef enum { PLAYER_ONE, PLAYER_TWO } Player;

const std::bitset<88> arm_bits(
    "11111000011111000011111000111110000111110001111100011111001111100011111000"
    "11110000111000");
const std::bitset<551> body_bits(
    "01111100000001111101111100000000011111111110000000001111111111000000000111"
    "11111110000000001111111111000000000111110111110000000111110011111100000111"
    "11100001111111111111000000111111111111000000011111111111100000001111111111"
    "11000000011111111111100000001111111111110000011000111110001110001111101110"
    "11111100011111101011111110001111111011111111000111111101111111100011111111"
    "11111111000111111111111111100000000111111000000000000111111110000000000011"
    "11111100000000001100100111000000000111111111100000000011000000110000000000"
    "111111110000000000011111111000000");

static uint8_t* level_data = NULL;
static int16_t angle;
static int16_t velocity;
static Player current_player;
static GameState state;

static uint16_t input;

static bool allocate_level_data() {
    uint16_t width = video_out.width();
    uint16_t height = video_out.height();

    level_data = (uint8_t*)malloc((uint32_t)width * height);

    return level_data != NULL;
}

static void reset_game() {
    if (level_data != NULL) {
        free(level_data);
        level_data = NULL;
    }

    current_player = PLAYER_ONE;
    state = GS_READ_ANGLE;
    input = 0;

    allocate_level_data();
}

static void copy_framebuffer_into_level() {
    if (level_data == NULL) return;

    uint8_t** framebuffer = video_framebuffer_lines();
    if (framebuffer == NULL) return;

    uint16_t width = video_out.width();
    uint16_t height = video_out.height();

    for (uint16_t y = 0; y < height; y++) {
        std::memcpy(level_data + (uint32_t)y * width, framebuffer[y], width);
    }
}

static void copy_level_into_framebuffer() {
    if (level_data == NULL) return;

    uint8_t** framebuffer = video_framebuffer_lines();
    if (framebuffer == NULL) return;

    uint16_t width = video_out.width();
    uint16_t height = video_out.height();

    for (uint16_t y = 0; y < height; y++) {
        std::memcpy(framebuffer[y], level_data + (uint32_t)y * width, width);
    }
}

template <std::size_t N>
static void draw_bitset(const std::bitset<N>* data, int16_t x, int16_t y,
                        uint8_t width, uint8_t height = 0, bool flip_x = false,
                        bool flip_y = false) {
    for (uint16_t i = 0; i < data->size(); ++i) {
        uint8_t xx = i % width;
        uint8_t yy = i / width;

        if (flip_x) xx = width - 1 - xx;
        if (flip_y) yy = height - 1 - yy;

        if (data->test(i)) {
            video_out.drawPixel(x + xx, y + yy, 255);
        }
    }
}

static void draw_gorilla(int16_t x, int16_t y, bool left_arm_up,
                         bool right_arm_up) {
    draw_bitset(&body_bits, x, y, GORILLA_BODY_WIDTH);

    int8_t left_arm_offset = left_arm_up ? -1 : 9;
    int8_t right_arm_offset = right_arm_up ? -1 : 9;

    draw_bitset(&arm_bits, x - 4, y + left_arm_offset, GORILLA_ARM_WIDTH,
                GORILLA_ARM_HEIGHT, false, left_arm_up);

    draw_bitset(&arm_bits, x + GORILLA_BODY_WIDTH - 3, y + right_arm_offset,
                GORILLA_ARM_WIDTH, GORILLA_ARM_HEIGHT, true, right_arm_up);
}

// TODO: lit and unlit windows
static void draw_house(int16_t x, int16_t y, uint8_t width, uint8_t height,
                       uint8_t color) {
    video_out.fillRect(x, y, width, height, color);

    // Draw windows
    for (uint8_t xx = 3; xx < width - 8; xx += 9) {
        for (uint8_t yy = 3; yy < height - 8; yy += 13) {
            uint8_t c = random(0, 2) > 1 ? 10 : 0;

            video_out.fillRect(x + xx, y + yy, 5, 10, c);
        }
    }
}

static void draw_houses() {
    uint16_t x = 0;

    while (x < video_out.width()) {
        uint8_t width = random(30, 61);
        uint8_t height = random(30, 150);
        // Upper bound is 255 exclusive. 255 is reserved for gorillas
        uint8_t color = random(200, 255);

        draw_house(x, video_out.height() - height, width, height, color);

        x += width + 2;
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

static void place_gorilla(int16_t x) {
    int16_t y = 0;

    uint8_t** framebuffer = video_framebuffer_lines();
    while (framebuffer[y][x + GORILLA_BODY_WIDTH / 2] == 0) {
        y++;
    }

    draw_gorilla(x, y - GORILLA_BODY_HEIGHT, false, false);
}

static void place_gorillas() {
    place_gorilla(10);
    place_gorilla(video_out.width() - 40);
}

void gorillas_enter() {
    reset_game();

    video_out.fillScreen(0);
    draw_houses();
    place_gorillas();
    copy_framebuffer_into_level();
}

void gorillas_exit() { reset_game(); }

void gorillas_render() {
    video_out.waitForFrame();
    video_out.fillScreen(0);
    video_out.setTextSize(1);
    video_out.setTextColor(0xFF);

    copy_level_into_framebuffer();

    copy_framebuffer_into_level();

    video_out.setCursor(0, TEXT_Y_START);
    draw_read_angle();
    draw_read_velocity();
}

void gorillas_handle_key(char read_key) {
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
        }

        input = 0;
    }
}
