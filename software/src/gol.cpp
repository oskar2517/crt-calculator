#include <bitset>

#include "video.h"

#define BOARD_WIDTH 128
#define BOARD_HEIGHT 120

static std::bitset<BOARD_WIDTH * BOARD_HEIGHT> board;
static std::bitset<BOARD_WIDTH * BOARD_HEIGHT> next_board;

static uint8_t neighbors(int16_t x, int16_t y) {
    uint8_t n = 0;

    for (int8_t dx = -1; dx <= 1; dx++) {
        for (int8_t dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;

            uint16_t xx = (x + dx + BOARD_WIDTH) % BOARD_WIDTH;
            uint16_t yy = (y + dy + BOARD_HEIGHT) % BOARD_HEIGHT;

            if (board.test(yy * BOARD_WIDTH + xx)) {
                n++;
            }
        }
    }

    return n;
}

static void next_generation() {
    next_board.reset();

    for (uint16_t i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        int16_t x = i % BOARD_WIDTH;
        int16_t y = i / BOARD_WIDTH;
        bool alive = board.test(i);
        uint8_t n = neighbors(x, y);

        if ((alive && (n == 2 || n == 3)) || (!alive && n == 3)) {
            next_board.set(i);
        }

        // Extra rule to prevent stabilization
        if (n >= 1 && random(0, 1000) == 999) {
            next_board.flip(i);
        }
    }

    board = next_board;
}

static void draw_board() {
    for (uint16_t i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        if (!board.test(i)) continue;

        int16_t x = i % BOARD_WIDTH * 2;
        int16_t y = i / BOARD_WIDTH * 2;

        video_out.fillRect(x, y, 2, 2, 0xFF);
    }
}

static void generate_random_board() {
    for (uint16_t i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        if (random(0, 2) > 0) {
            board.set(i);
        }
    }
}

void gol_enter() { generate_random_board(); }

void gol_exit() {}

void gol_render() {
    next_generation();

    video_out.waitForFrame();
    video_out.fillScreen(0);

    if (board.none()) {
        generate_random_board();
    }

    draw_board();
}

void gol_handle_key(char read_key) { generate_random_board(); }
