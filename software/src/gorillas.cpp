#include <bitset>

#include "video.h"

#define GORILLA_BODY_WIDTH 19
#define GORILLA_BODY_HEIGHT 29

#define GORILLA_ARM_WIDTH 8
#define GORILLA_ARM_HEIGHT 11

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

template <std::size_t N>
void draw_bitset(const std::bitset<N>* data, int16_t x, int16_t y,
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

void draw_gorilla(int16_t x, int16_t y, bool left_arm_up, bool right_arm_up) {
    draw_bitset(&body_bits, x, y, GORILLA_BODY_WIDTH);

    int8_t left_arm_offset = left_arm_up ? -1 : 9;
    int8_t right_arm_offset = right_arm_up ? -1 : 9;

    draw_bitset(&arm_bits, x - 4, y + left_arm_offset, GORILLA_ARM_WIDTH,
                GORILLA_ARM_HEIGHT, false, left_arm_up);

    draw_bitset(&arm_bits, x + GORILLA_BODY_WIDTH - 3, y + right_arm_offset,
                GORILLA_ARM_WIDTH, GORILLA_ARM_HEIGHT, true, right_arm_up);
}

void gorillas_enter() {}

void gorillas_exit() {}

void gorillas_render() {
    video_out.waitForFrame();
    video_out.fillScreen(0);

    static uint32_t last_change_millis = millis();
    static bool left_arm_up = true;
    static bool right_arm_up = false;

    if (millis() - last_change_millis > 500) {
        last_change_millis = millis();
        left_arm_up ^= 1;
        right_arm_up ^= 1;
    }

    draw_gorilla(30, 30, left_arm_up, right_arm_up);
}

void gorillas_handle_key(char read_key) {}