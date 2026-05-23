#include <LittleFS.h>
#include <stdint.h>

#include "video.h"

#define OFFSET_X(x) ((x) + 15)
#define OFFSET_Y(y) ((y) + 40)

static File file;
static uint8_t buffer[5096];
static uint32_t buffer_len;
static uint32_t last_frame_read;

void read_frame_into_buffer() {
    uint32_t len = 0;

    file.read((uint8_t*)&len, sizeof(len));

    size_t n = file.read(buffer, len);

    buffer_len = n;

    /* if (buffer_len == 0) {
        file.seek(0);
        read_frame_into_buffer();
    } */
}

void bad_apple_enter() {
    file = LittleFS.open("/bad_apple.bin", "r");
    last_frame_read = 0;
}

void bad_apple_exit() {
    file.close();
}

void bad_apple_render() {
    static uint8_t tick = 0;

    video_out.waitForFrame();

    tick += 24;
    if (tick >= 50) {
        tick -= 50;
        read_frame_into_buffer();
    }

    video_out.fillScreen(0);

    for (uint32_t i = 0; i + 3 < buffer_len; i += 2) {
        uint8_t a = buffer[i];
        uint8_t b = buffer[i + 1];
        uint8_t c;
        uint8_t d;
        if (buffer[i + 2] == 255) {
            c = buffer[i + 3];
            d = buffer[i + 4];
            i += 3;
        } else {
            c = buffer[i + 2];
            d = buffer[i + 3];
        }

        video_out.drawLine(
            OFFSET_X(a),
            OFFSET_Y(b),
            OFFSET_X(c),
            OFFSET_Y(d),
            0xFF
        );
    }
}

void bad_apple_handle_key(char read_key) {}
